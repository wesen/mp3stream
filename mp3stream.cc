/*
 * mp3stream - encode /dev/dsp and send it to a shoutcast server
 * 2005 - bl0rg.net - public domain
 *
 * mp3stream.cc - main file, contains command-line option decoding and
 * mainloop.
 */

#include <assert.h>
#include <unistd.h>

#include <iostream>
#include <list>
#include <string>
#include <map>
using namespace std;

#include "Error.h"
#include "AudioCard.h"
#include "Streamer.h"
#include "Url.h"
#include "LameEncoder.h"

static const string usageString =
"Usage:\n\n"
"mp3stream [-h] [-b bitrate] [-n name] [-g genre] [-p publicstr]\n"
"          [-d description] [-c contentid] [-u url] [-t streamurl] [-v]\n\n"
"-b bitrate: set the bitrate of the stream in kbps (default 128 kbps)\n"
"-v: show visual feedback while encoding\n\n"
"To send streams to multiple url, specify one or more streamurls.\n"
"The stream description will consist of the last parameters given.\n"
"For example: \n\n"
"mp3stream -b 128 -n \"128 kbps\" -t xaudio://localhost:8001/stream128 \n"
"          -b 64 -n \"64 kbps\" -t xaudio://localhost:8001/stream64\n\n"
"will send a 128 kbps encoded stream to the icecast server under the\n"
"mountpoint /stream128 and a 64 kbps encoded stream to the icecast server\n"
"under the mountpoint /stream64.\n\n"
"mp3stream can also log the encoded data to a file by using a file:/// URL.\n"
"For example: \n\n"
"mp3stream -b 128 -n \"128 kbps\" -t xaudio://localhost/bla -t file:///var/mp3-log.mp3\n\n"
"will send the 128 kbps encoded data to localhost and log it into mp3-log.mp3.\n";

static void usage() {
  cerr << usageString;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage();
    return 1;
  }

  unsigned long bitrate = 128;
  string name = "";
  string genre = "";
  string publicStr = "";
  string description = "";
  string url = "";
  string contentid = "";
  list<Streamer *> streamerList;
  int retval = 0;
  bool bDisplay = false;

  int c;
  while ((c = getopt(argc, argv, "hb:n:g:p:d:c:u:t:v")) >= 0) {
    switch (c) {
    case 'h':
      usage();
      return 0;

    case 'v':
      bDisplay = true;
      break;

    case 'b':
      bitrate = atoi(optarg);
      break;

    case 'n':
      name = string(optarg);
      break;

    case 'g':
      genre = string(optarg);
      break;

    case 'p':
      publicStr = string(optarg);
      break;

    case 'd':
      description = string(optarg);
      break;

    case 'c':
      contentid = string(optarg);
      break;

    case 'u':
      url = string(optarg);
      break;

    case 't':
      try {
	Stream stream(bitrate, name, genre, url,
		      publicStr, description, contentid);
	Url streamUrl(optarg);
	Streamer *streamer = new Streamer(stream, streamUrl);
	streamerList.push_front(streamer);
      } catch (Error &e) {
	cerr << "Could not parse URL \"" << optarg << "\": "
	     << e.getMessage() << endl;
	retval = 1;
	goto exit;
      }
      break;

    default:
      cerr << "Unknown switch " << c << endl;
      usage();
      return 1;
    }
  }

  try {
    AudioCard *audioCard = NULL;
    map<LameEncoder *,list<Streamer *> > encoderMap;

    try {
      audioCard = new AudioCard(false);
    } catch (AudioError &e) {
      cerr << "Could not initialize soundcard: " << e.getMessage() << endl;
      retval = 1;
      goto exit;
    }

    for (list<Streamer *>::iterator pStreamer = streamerList.begin();
	 pStreamer != streamerList.end(); pStreamer++) {
      Streamer &streamer = *(*pStreamer);
      unsigned long bitrate = streamer.getBitrate();

      map<LameEncoder *,list<Streamer *> >::iterator p;
      for (p = encoderMap.begin();
	   p != encoderMap.end(); p++) {
	const LameEncoder &encoder = *(p->first);
	list<Streamer *> &listStreamer = p->second;
	if (encoder.getBitrate() == bitrate) {
	  listStreamer.push_back((*pStreamer));
	  break;
	}
      }
      if (p == encoderMap.end()) {
	try {
	  LameEncoder *encoder = new LameEncoder(*audioCard, bitrate);
	  list<Streamer *> &listStreamer = encoderMap[encoder];
	  listStreamer.push_back((*pStreamer));
	} catch (LameError &e) {
	  cerr << "Could not create an encoder with " << bitrate << " kbps: "
	       << endl
	       << "        " << e.getMessage() << endl;
	  retval = 1;
	  goto cleanup;
	}
      }
    }

    /* Print information about setup */
    cerr << endl
	 << "Streaming setup:" << endl
	 << "----------------" << endl;
    
    for (map<LameEncoder *,list<Streamer *> >::iterator p = encoderMap.begin();
	 p != encoderMap.end(); p++) {
      LameEncoder *encoder = p->first;
      list<Streamer *> &l = p->second;

      cerr << "Streaming " << encoder->getBitrate()
	   << " kbps stream to: " << endl;
      
      for (list<Streamer *>::iterator pStreamer = l.begin();
	   pStreamer != l.end(); pStreamer++) {
	cerr << "  " << (*pStreamer)->getUrl()
	     << " " << (*pStreamer)->getInfo() << endl;
      }

      cerr << endl;
    }

    for (list<Streamer *>::iterator pStreamer = streamerList.begin();
	 pStreamer != streamerList.end(); pStreamer++) {
      Streamer &streamer = *(*pStreamer);
      cerr << "Logging in to " << streamer.getUrl() << "... ";
      try {
	streamer.Login();
      } catch (Error &e) {
	cerr << endl;
	cerr << "       " << e.getMessage() << endl;
	retval = 1;
	goto cleanup;
      }
      cerr << "OK" << endl;
    }

    cerr << "Streaming..." << endl;
    
    /* main loop */
    static short buf[rawDataBlockSize / 2];
    static unsigned char mp3Buf[rawDataBlockSize];
    for (;;) {
      audioCard->Read(buf);

      static int count = 0;
      if (bDisplay && ((count++ % 5) == 0)) {
	unsigned short maxVol = 0;
	for (unsigned int i = 0; i < rawDataBlockSize / 2; i++) {
	  if (abs(buf[i]) > maxVol)
	    maxVol = abs(buf[i]);
	}
	cout << "\r                                                                                     ";
	cout << "\rLevel: ";
	for (unsigned short j = 0; j <= maxVol; j += 550)
	  cout << "=";
	if (maxVol > 32000)
	  cout << "*clip*";
	cout.flush();
      }
      
      for (map<LameEncoder *,list<Streamer *> >::iterator p = encoderMap.begin();
	   p != encoderMap.end(); p++) {
	LameEncoder *encoder = p->first;
	list<Streamer *> &l = p->second;

	int ret = encoder->EncodeBuffer(buf, rawDataBlockSize / 2,
				       mp3Buf, sizeof(mp3Buf));

	if (ret > 0) {
	  for (list<Streamer *>::iterator pStreamer = l.begin();
	       pStreamer != l.end(); pStreamer++) {
	    Streamer &streamer = *(*pStreamer);
	    streamer.Write(mp3Buf, ret);
	  }
	}
      }
    }

  cleanup:
    for (map<LameEncoder *,list<Streamer *> >::iterator p = encoderMap.begin();
	 p != encoderMap.end(); p++) {
      LameEncoder *encoder = p->first;
      delete encoder;
    }
    delete audioCard;

  } catch (Error &e) {
    cerr << e.getMessage() << endl;
    retval = 1;
    goto exit;
  }

 exit:
  for (list<Streamer *>::iterator pStreamer = streamerList.begin();
       pStreamer != streamerList.end(); pStreamer++) {
    delete (*pStreamer);
  }

  return retval;
}
