#ifndef LAME_ENCODER_H__
#define LAME_ENCODER_H__

#include <lame/lame.h>
#include <string>

#include "Error.h"
#include "AudioCard.h"

class LameError : public Error {
public:
  LameError(std::string msg) : Error(msg) { }
  virtual ~LameError() { }
};

class LameEncoder {
protected:
  lame_global_flags *gf;
  unsigned long bitrate;
  
public:
  LameEncoder(const AudioCard &card,
	      unsigned long bitrate = 128,
	      int quality = 5);
  ~LameEncoder();

  unsigned long getBitrate() const { return bitrate; }
  
  int EncodeBuffer(short input[],
		   unsigned long inputSize,
		   unsigned char output[],
		   unsigned long outputSize);
};

#endif // LAME_ENCODER_H__
