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

static void usage(char *program) {
  cerr << "Usage: " << program;
  cerr << " [-h] ";
  cerr << "[-b bitrate] [-n name] [-g genre] [-p publicstr] ";
  cerr << "[-d description] [-c contentid] [-u url] ";
  cerr << "[-t streamurl]" << endl;
  cerr << endl << "To send streams to multiple url, specify one or more streamurls." << endl;
  cerr << "The stream description will consist of the last parameters given." << endl;
  cerr << "For example: " <<endl;
  cerr << program << " -b 128 -n \"128 kbps stream\" -t xaudio://localhost:8001/stream128 -b 64 -n \"64 kbps stream\" -t xaudio://localhost:8001/stream64" << endl;
  cerr <<
    "will send a 128 kbps encoded stream to the icecast server under the mountpoint " << endl <<
    "/stream128 and a 64 kbps encoded stream to the icecast server under the mount /stream64." << endl;
  // XXX defaults
}

class StreamerEncoderPair {
public:
  Streamer &streamer;
  LameEncoder &encoder;

  StreamerEncoderPair(Streamer &streamer, LameEncoder &encoder) :
    streamer(streamer), encoder(encoder) {
  }
  ~StreamerEncoderPair() { }
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
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

  int c;
  while ((c = getopt(argc, argv, "hb:n:g:p:d:c:u:t:")) >= 0) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      return 0;

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
	return 1;
      }
      break;

    default:
      cerr << "Unknown switch " << c << endl;
      usage(argv[0]);
      return 1;
    }
  }

  try {
    AudioCard audioCard(false);
    map<LameEncoder *,list<Streamer *> > encoderMap;

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
	  cout << "adding stream" << endl;
	  listStreamer.push_back((*pStreamer));
	  break;
	}
      }
      if (p == encoderMap.end()) {
	LameEncoder *encoder = new LameEncoder(audioCard, bitrate);
	cout << "Creating new encoder with bitrate " << bitrate << endl;
	list<Streamer *> &listStreamer = encoderMap[encoder];
	listStreamer.push_back((*pStreamer));
      }
    }

    for (list<Streamer *>::iterator pStreamer = streamerList.begin();
	 pStreamer != streamerList.end(); pStreamer++) {
      Streamer &streamer = *(*pStreamer);
      cout << "Logging in to streamer with host " << streamer.getHost() << endl;
      streamer.Login();
      cout << "Logged in to streamer with host " << streamer.getHost() << endl;
    }

    /* main loop */
    static unsigned char buf[rawDataBlockSize];
    static unsigned char mp3Buf[rawDataBlockSize];
    for (;;) {
      audioCard.Read(buf);
      for (map<LameEncoder *,list<Streamer *> >::iterator p = encoderMap.begin();
	   p != encoderMap.end(); p++) {
	LameEncoder *encoder = p->first;
	list<Streamer *> &l = p->second;

	int ret = encoder->EncodeBuffer(buf, sizeof(buf),
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

    /* XXX try catch an der richtigen stelle bitte */
    
    /* cleanup */
    map<LameEncoder *,list<Streamer *> >::iterator p;
    for (p = encoderMap.begin();
	 p != encoderMap.end(); p++) {
      LameEncoder *encoder = p->first;
      list<Streamer *> &l = p->second;
      cout << "Encoder has " << l.size() << " streams" << endl;
      cout << "Delete encoder with bitrate " << encoder->getBitrate() << endl;
      delete encoder;
    }

  } catch (Error &e) {
    cerr << e.getMessage() << endl;

    for (list<Streamer *>::iterator pStreamer = streamerList.begin();
	 pStreamer != streamerList.end(); pStreamer++) {
      delete (*pStreamer);
    }
    return 1;
  }

  for (list<Streamer *>::iterator pStreamer = streamerList.begin();
       pStreamer != streamerList.end(); pStreamer++) {
    delete (*pStreamer);
  }

  return 0;
}
