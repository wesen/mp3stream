/*
 * mp3stream - encode /dev/dsp and send it to a shoutcast server
 * 2005 - bl0rg.net - public domain
 *
 * AudioCard.h - audio card abstraction
 */

#ifndef AUDIOCARD_H__
#define AUDIOCARD_H__

#include <string>

#include "Error.h"

/* The block size that is read from /dev/dsp. */
static const unsigned int rawDataBlockSize = 4096;

/* Errors thrown when something goes wrong with audio handling. */
class AudioError : public Error {
public:
  AudioError(std::string msg) : Error(msg) { }
  virtual ~AudioError() { }
};

/* Audio card abstraction, basically used to read a block of data from
   the soundcard. The data read is converted to signed 16 bits
   values. */
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

