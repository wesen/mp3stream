#include "LameEncoder.h"

using namespace std;

LameEncoder::LameEncoder(const AudioCard &audioCard, unsigned long bitrate,
			 bool bSwapBytes, int quality) :
  gf(NULL), bSwapBytes(bSwapBytes), bitrate(bitrate) {
  gf = lame_init();
  if (gf == NULL)
    throw LameError("Could not initialize lame library");
  
  lame_set_num_channels(gf, audioCard.getStereo() ? 2 : 1);
  lame_set_in_samplerate(gf, audioCard.getSampleRate());
  lame_set_quality(gf, quality);
  lame_set_mode(gf, audioCard.getStereo() ? STEREO : MONO);
  lame_set_brate(gf, bitrate);
  
  int ret = lame_init_params(gf);
  if (ret < 0)
    throw LameError("Could not initialize additionals params for lame");
}

LameEncoder::~LameEncoder() {
  if (gf != NULL)
    lame_close(gf);
}

int LameEncoder::EncodeBuffer(unsigned char input[],
			      unsigned long inputSize,
			      unsigned char output[],
			      unsigned long outputSize) {
  int ret;

  if (gf == NULL)
    throw LameError("Lame encoder not initialized");

  unsigned long numSamples = inputSize / 2 / lame_get_num_channels(gf);
  short sBuf[inputSize / 2];
  for (unsigned int i = 0; i < inputSize / 2; i++) {
    if (bSwapBytes)
      sBuf[i] = (input[i*2] & 0xFF) | ((input[i*2+1] & 0xFF) << 8);
    else
      sBuf[i] = (input[i*2+1] & 0xFF) | ((input[i*2] & 0xFF) << 8);
  }
  
  ret = lame_encode_buffer_interleaved(gf, sBuf, numSamples,
				       output, outputSize);
  
  if (ret < 0) {
    switch (ret) {
    case -1:
      throw LameError("mp3buf was too small");
    case -2:
      throw LameError("malloc() problem");
    case -3:
      throw LameError("lame_init_param() not called");
    case -4:
      throw LameError("psychoacoustics problems");
    default:
      throw LameError("Unknown error");
    }
  }

  return ret;
}

