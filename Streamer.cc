#include <string>
#include <sstream>
using namespace std;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>

#include "Streamer.h"
#include "Url.h"

Stream::Stream(unsigned long bitrate,
	       const string &name, const string &genre,
	       const string &url, const string &publicStr,
	       const string &description, const string &contentid) :
  bitrate(bitrate), name(name), genre(genre), url(url),
  publicStr(publicStr), description(description),
  contentid(contentid) {
}

Stream::~Stream() {
}

Streamer::Streamer(const Stream &stream, const Url &url) :
  fd(-1), bConnected(false), bLoggedIn(false),
  stream(stream) {
  string protocol = url.getProtocol();
  if ((protocol != "icy") &&
      (protocol != "xaudio") &&
      (protocol != "file")) {
    throw StreamerError("Unknown protocol " + protocol);
  }
  
  host = url.getHost();
  port = url.getPort();
  if ((port == 0) && (protocol != "file"))
    throw StreamerError("No port was specified in the url.");
  path = url.getPath();
  password = url.getLogin();
  if (protocol == "icy") {
    type = IcyLoginType;
  } else if (protocol == "xaudio") {
    type = XAudioLoginType;
  } else if (protocol == "file") {
    type = FileLoginType;
  } else {
    type = NoneLoginType;
  }
}

Streamer::Streamer(const Stream &stream,
		   const string &host, unsigned short port,
		   const string &path, const string &password,
		   StreamerLoginType type) :
  fd(-1), bConnected(false), bLoggedIn(false),
  stream(stream),
  host(host), port(port), path(path),
  password(password), type(type) {
}

Streamer::~Streamer() {
  if (fd > 0) {
    int ret = close(fd);
    if (ret < 0)
      throw StreamerError("Could not close open connection (close): " +
			  string(strerror(errno)));
    fd = -1;
    bConnected = false;
    bLoggedIn = false;
  }
}

void Streamer::OpenFile() {
  if (fd != -1)
    throw StreamerError("Streamer already connected");

  fd = open(path.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
  if (fd < 0)
    throw StreamerError("Could not open file " + path);
  bConnected = true;
  bLoggedIn = true;
}

void Streamer::Connect() {
  if (fd != -1)
    throw StreamerError("Streamer already connected");

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    throw StreamerError("Could not create socket (socket): " +
			string(strerror(errno)));

  try {
    struct hostent *hostent;
    hostent = gethostbyname(host.c_str());
    if (hostent == NULL)
      throw StreamerError("Could not resolve address: " + host +
			  " (gethostbyname) : " +
			  string(strerror(h_errno)));
    
    if (hostent->h_addrtype != AF_INET)
      throw StreamerError("Only Ipv4 support for now");
    
    struct sockaddr_in sin;
    sin.sin_family = (short)hostent->h_addrtype;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = *(unsigned long *)hostent->h_addr;
    
    int ret = connect(fd, (struct sockaddr *)&sin, sizeof(sin));
    if (ret < 0)
      throw StreamerError("Could not connect to " + host + " (connect): " +
			  string(strerror(errno)));

    bConnected = true;
  } catch (Error &e) {
    close(fd);
    fd = -1;
    bConnected = false;
    bLoggedIn = false;
    throw e;
  }
}

void Streamer::Login() {

  switch (type) {
  case NoneLoginType:
    break;

  case IcyLoginType:
    if (!bConnected)
      Connect();
    assert(bConnected);
    IcyLogin();
    break;

  case XAudioLoginType:
    if (!bConnected)
      Connect();
    assert(bConnected);
    XAudioLogin();
    break;

  case FileLoginType:
    if (!bConnected)
      OpenFile();
    assert(bConnected);
    break;

  default:
    throw StreamerError("Unknown LoginType");
  }
  
  bLoggedIn = true;
}

void Streamer::IcyLogin() {
  Write((const unsigned char *)password.data(), password.size());
  Write((const unsigned char *)"\n", 1);
  
  unsigned char buf[3];
  Read(buf, sizeof(buf));
  if (! ((buf[0] == 'O') || (buf[0] == 'o')) &&
      ((buf[1] == 'K') || (buf[1] == 'k')))
    throw StreamerError("Could not connect to ICY server (did not receive OK response)");
  
  ostringstream ost;

  ost << "icy-br: " << stream.getBitrate() << "\n";
  ost << "icy-name: " << stream.getName() << "\n";
  ost << "icy-genre: " << stream.getGenre() << "\n";
  ost << "icy-url: " << stream.getUrl() << "\n";
  ost << "icy-pub: " << stream.getPublicStr() << "\n";
  ost << "\n";

  string IcyStr = ost.str();
  Write((const unsigned char *)IcyStr.data(), IcyStr.size());
}

void Streamer::XAudioLogin() {
  ostringstream ost;

  ost << "SOURCE " << password;
  ost << " " << path << "\n\n";
  ost << "x-audiocast-bitrate: " << stream.getBitrate() << "\n";
  ost << "x-audiocast-name: " << stream.getName() << "\n";
  ost << "x-audiocast-genre: " << stream.getGenre() << "\n";
  ost << "x-audiocast-url: " << stream.getUrl() << "\n";
  ost << "x-audiocast-public: " << stream.getPublicStr() << "\n";
  ost << "x-audiocast-description: " << stream.getDescription() << "\n";
  ost << "x-audiocast-contentid: " << stream.getContentid() << "\n";
  ost << "\n";

  string XAudioStr = ost.str();
  Write((const unsigned char *)XAudioStr.data(), XAudioStr.size());
}

void Streamer::Write(const unsigned char *buf, unsigned int len) {
  assert(bConnected);

  const unsigned char *p = buf;
  
 again:
  int ret = write(fd, p, len);
  if (ret < 0) {
    if (errno == EAGAIN)
      goto again;
    else
      throw StreamerError("Could not send buffer to remote side (write): " +
			  string(strerror(errno)));
  }
  if (ret == 0)
    throw StreamerError("Remote side closed the connection");
  len -= ret;
  p += ret;
  if (len > 0)
    goto again;
}

void Streamer::Read(unsigned char *buf, unsigned int len) {
  assert(bConnected);

  unsigned char *p = buf;
  
 again:
  int ret = read(fd, p, len);
  if (ret < 0) {
    if (errno == EAGAIN)
      goto again;
    else
      throw StreamerError("Could not read buffer from remote side (write): " +
			  string(strerror(errno)));
  }
  if (ret == 0)
    throw StreamerError("Remote side closed the connection");
  len -= ret;
  p += ret;
  if (len > 0)
    goto again;
}
