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
#include <unistd.h>

#include <linux/soundcard.h>

#include <string>

using namespace std;

#include "AudioCard.h"

AudioCard::AudioCard(bool fullDuplex, bool stereo,
		     unsigned long sampleRate, string path) :
  fullDuplex(fullDuplex), stereo(stereo), sampleRate(sampleRate), path(path) {
  fd = open(path.c_str(), O_RDWR);
  if (fd < 0)
    throw AudioError("Could not open soundcard (\"" + path + "\"): " + string(strerror(errno)));

  SetFormat();
  SetFullDuplex();
  SetStereo();
  SetSampleRate();
}

AudioCard::~AudioCard() {
  if (fd >= 0) {
    int ret;

    ret = close(fd);
    if (ret < 0)
      throw AudioError("Could not close soundcard (close): " + string(strerror(errno)));
    fd = -1;
  }
}

void AudioCard::SetFullDuplex() {
  assert(fd > 0);

  unsigned int audio_caps;
  int ret;
  if (fullDuplex) {
    ret = ioctl(fd, SNDCTL_DSP_GETCAPS, &audio_caps);
    if (ret < 0)
      throw AudioError("Could not get SND capabilities (ioctl): " + string(strerror(errno)));
    
    if (!(audio_caps & DSP_CAP_DUPLEX))
      throw AudioError("Soundcard does not support fullduplex");
    
    ret = ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0);
    if (ret < 0)
      throw AudioError("Could not set fullDuplex (ioctl): " + string(strerror(errno)));
  }
}

void AudioCard::SetFormat() {
  assert(fd > 0);

  unsigned int format;
  int ret = ioctl(fd, SNDCTL_DSP_GETFMTS, &format);
  if (ret < 0)
    throw AudioError("Could not get supported sound formats (ioctl): " + string(strerror(errno)));
  
  if (!(format & AFMT_S16_LE))
    throw AudioError("Soundcard does not support 16 bit little-endian mode");

  format = AFMT_S16_LE;
  ret = ioctl(fd, SNDCTL_DSP_SETFMT, &format);
  if (ret < 0)
    throw AudioError("Could not set soundcard format to 16 bit little-endian (ioctl): " +
		     string(strerror(errno)));
}

void AudioCard::SetStereo() {
  assert(fd > 0);

  int stereo_arg = stereo ? 1 : 0;
  int ret = ioctl(fd, SNDCTL_DSP_STEREO, &stereo_arg);
  if (ret < 0)
    throw AudioError("Could not set stereo setting (ioctl): " +
		     string(strerror(errno)));
  
}
void AudioCard::SetSampleRate() {
  assert(fd > 0);

  unsigned long speed = sampleRate;
  int ret = ioctl(fd, SNDCTL_DSP_SPEED, &speed);
  if (ret < 0)
    throw AudioError("Could not set sample rate (ioctl): " +
		     string(strerror(errno)));
}

void AudioCard::Read(short sBuf[rawDataBlockSize / 2]) {
  assert(fd > 0);

  unsigned char buf[rawDataBlockSize];

  int ret = read(fd, buf, rawDataBlockSize);
  if ((ret < 0) || ((unsigned int)ret != rawDataBlockSize))
    throw AudioError("Could not read from the soundcard (read): " +
		     string(strerror(errno)));
  if (fullDuplex) {
    ret = write(fd, buf, rawDataBlockSize);
    if ((ret < 0) || ((unsigned int)ret != rawDataBlockSize))
      throw AudioError("Could not write to the soundcard (write): " +
		       string(strerror(errno)));
  }

  for (unsigned int i = 0; i < rawDataBlockSize / 2; i++) {
    /* we only support 16LE format */
    sBuf[i] = (buf[i*2] & 0xFF) | ((buf[i*2+1] & 0xFF) << 8);
  }
}

