#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include <assert.h>
#include <string.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "Error.h"
#include "AudioCard.h"

/* Errors */
class Mp3Error : public Error {
public:
  Mp3Error(string msg) : Error(msg) { }
  virtual ~Mp3Error() { }
};

class EncoderError : public Error {
public:
  EncoderError(string msg) : Error(msg) { }
  virtual ~EncoderError() { }
};


/* Mp3File */
#include "mp3.h"
#include "file.h"

class Mp3File {
protected:
  file_t file;

public:
  Mp3File(string path);
  Mp3File(int fd);
  ~Mp3File();

  unsigned long CopyFrameAndReturnDuration(file_t *tofile);

protected:
  mp3_frame_t frame;
  void ReadFrame();
};

class Mp3FileFinished {
public:
  Mp3FileFinished() { }
  ~Mp3FileFinished() { }
};

Mp3File::Mp3File(string path) {
  if (!file_open_read(&file, path.c_str())) {
    throw Mp3Error("Could not open \"" + path + "\" (open): " + string(strerror(errno)));
  }
}

Mp3File::Mp3File(int fd) {
  if (!file_open_fd(&file, fd)) {
    throw Mp3Error("Could not open (open): " + string(strerror(errno)));
  }
}

Mp3File::~Mp3File() {
  file_close(&file);
}

void Mp3File::ReadFrame() {
  int ret = mp3_next_frame(&file, &frame);
  if (ret == EEOF)
    throw Mp3FileFinished();
  if (ret < 0)
    throw Mp3Error("Could not read next frame: " + string(strerror(errno)));
}

unsigned long Mp3File::CopyFrameAndReturnDuration(file_t *tofile) {
  ReadFrame();
  unsigned int ret = file_write(tofile, frame.raw, frame.frame_size);
  if (ret != frame.frame_size)
    throw Mp3Error("Could not copy frame: " + string(strerror(errno)));
  return frame.usec;
}

/* AudioCard */
#include <linux/soundcard.h>


/* Lame */

class LameEncoder {
protected:
  pid_t pid;

  AudioCard *card;
  unsigned long bitRate;
  string lameCommand;

  int rawDataFd;
  int mp3DataFd;
  
public:
  LameEncoder(AudioCard *card, unsigned long bitRate = 128000,
	      string lameCommand = "lame");
  ~LameEncoder();


  void WriteRawData(unsigned char buf[rawDataBlockSize]);
  unsigned long ReadMp3Data(unsigned char *buf, unsigned int maxSize);
  
protected:
  void StartEncoder();
  void StartEncoderChild(int inputFd, int outputFd, vector<string> &vargs);
};

LameEncoder::LameEncoder(AudioCard *card, unsigned long bitRate, string lameCommand) :
  pid(-1), card(card), bitRate(bitRate), lameCommand(lameCommand),
  rawDataFd(-1), mp3DataFd(-1) {
  StartEncoder();
}

LameEncoder::~LameEncoder() {
  StopEncoder();
}

void LameEncoder::StopEncoder() {
  if (pid > 0) {
    int ret = kill(pid, SIGTERM);
    if (ret < 0)
      throw EncoderError("Could not kill (SIGTERM) running lame (kill): " +
			 string(strerror(errno)));

    static const int max_retries = 5;
    static const int retry_delay = 5000;
    for (int retries = 0; i < max_retries; i++) {
      ret = waitpid(pid, NULL, WNOHANG);
      if (ret < 0)
	throw EncoderError("Could not wait for the child to exit (waitpid): "
			   + string(strerror(errno)));
      if (ret == pid)
	return;
      usleep(retry_delay);
    }

    ret = kill(pid, SIGKILL);
    if (ret < 0)
      throw EncoderError("Could not kill (SIGKILL) running lame (kill): " +
			 string(strerror(errno)));
  }
}

void LameEncoder::StartEncoder() {
  char buf[256];
  vector<string> vargs;

  // stereo mode
  vargs.push_back("-m");
  vargs.push_back(card->getStereo() ? "j" : "m");

  // sample rate
  vargs.push_back("-s");
  snprintf(buf, sizeof(buf), "%f", card->getSampleRate / 1000.0);
  vargs.push_back(buf);

  // bit rate
  vargs.push_back("-b");
  snprintf(buf, sizeof(buf), "%d", bitrate / 1000.0);
  vargs.push_back(buf);

  // little endian
  unsigned short endianTest = 0xabcd;
  unsigned char *ptr = (unsigned char *)&endianTest;
  if (ptr[0] == 0xcd)
    vargs.push_back("-x");

  // read from stdin, write to stdout
  vargs.push_back("-");
  vargs.push_back("-");

  int encoderInput[2];
  int encoderOutput[2];

  int piperet = pipe(encoderInput);
  if (piperet < 0)
    throw EncoderError("Could not create input pipes for the MP3 encoder (pipe): " +
		       string(strerror(errno)));
  piperet = pipe(encoderOutput);
  if (piperet < 0) {
    close(encoderInput[0]);
    close(encoderInput[1]);
    throw EncoderError("Could not create output pipes for the MP3 encoder (pipe): " +
		       string(strerror(errno)));
  }

  if ((pid = fork()) == 0) {
    close(encoderInput[1]);
    close(encoderOutput[0]);

    try {
      StartEncoderChild(encoderInput[0], encoderOutput[1], vargs);
    } catch (...) {
      exit(1);
    }

    exit(1);
  } else {
    close(encoderInput[0]);
    close(encoderOutput[1]);

    rawDataFd = encoderInput[1];
    mp3DataFd = encoderOutput[0];
  }
}

void LameEncoder::StartEncoderChild(int inputFd, int outputFd, vector<string> &vargs) {
  int ret;
  
  close(0); close(1); close(2);
    
  /* set up stdin of the encoder */
  ret = dup2(inputFd, 0);
  if (ret < 0) {
    close(inputFd);
    close(outputFd);
    throw EncoderError("Could not duplicate the input pipe fd (dup2): " +
		       string(strerror(errno)));
  }
    
  ret = dup2(outputFd, 1);
  if (ret < 0) {
    close(inputFd);
    close(outputFd);
    throw EncoderError("Could not duplicate the input pipe fd (dup2): " +
		       string(strerror(errno)));
  }

  const char *args[vargs.size() + 1];
  for (int i = 0; i < vargs.size(); i++)
    args[i] = (vargs[i]).c_str();
  args[vargs.size()] = NULL;

  int ret = execvp(lameCommand.c_str(), args);
  if (ret < 0)
    throw EncoderError("Could not exec " + lameCommand + " (execvp): " +
		       string(strerror(errno)));
}

/* Main */
static const long long MAX_WAIT_TIME (1 * 1000 * 1000);

unsigned long long GetCurrentTimeMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[]) {
  try {
    AudioCard audio;
    file_t output;
    if (!file_open_write(&output, "/dev/null"))
      throw Mp3Error("Could not open /dev/null for writing: " + string(strerror(errno)));

    static long wait_time = 0;

    for (int i = 1; i < argc; i++) {
      cout << "Streaming " << argv[i] << "..." << endl;
      Mp3File file(argv[i]);
      try {
	for (;;) {
	  unsigned long long start = GetCurrentTimeMs();

	  wait_time += file.CopyFrameAndReturnDuration(&output);

	  if (wait_time > 1000)
	    usleep(wait_time);

	  wait_time -= GetCurrentTimeMs() - start;
	  if (abs(wait_time) > MAX_WAIT_TIME)
	    wait_time = 0;
	}
      } catch (Mp3FileFinished) {
      }
    }
  } catch (Error &e) {
    cout << "Caught an error while streaming: " << e.getMessage() << endl;
  }

  return 0;
}
