#ifndef AUDIOCARD_H__
#define AUDIOCARD_H__

#include <string>

#include "Error.h"

static const unsigned int rawDataBlockSize = 4096;

class AudioError : public Error {
public:
  AudioError(std::string msg) : Error(msg) { }
  virtual ~AudioError() { }
};

class AudioCard {
protected:
  int fd;
  bool fullDuplex;
  bool stereo;
  unsigned long sampleRate;
  std::string path;

  void SetFullDuplex();
  void SetFormat();
  void SetSampleRate();
  void SetStereo();
  
public:
  AudioCard(bool fullDuplex = true, bool stereo = true,
	    unsigned long sampleRate = 44100, std::string path = "/dev/dsp");
  ~AudioCard();

  bool getFullDuplex() const {
    return fullDuplex;
  }

  bool getStereo() const {
    return stereo;
  }

  unsigned long getSampleRate() const {
    return sampleRate;
  }

  void Read(short sBuf[rawDataBlockSize / 2]);
};

#endif // AUDIOCARD_H__

