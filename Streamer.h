#ifndef STREAMER_H__
#define STREAMER_H__

#include <string>

#include "Url.h"
#include "Error.h"

class StreamerError : public Error {
public:
  StreamerError(std::string msg) : Error(msg) { }
  virtual ~StreamerError() { }
};

class Stream {
 protected:
  unsigned long bitrate;
  std::string name;
  std::string genre;
  std::string url;
  std::string publicStr;
  std::string description;
  std::string contentid;

 public:
  Stream(const unsigned long bitrate,
	 const std::string &name = "",
	 const std::string &genre = "",
	 const std::string &url = "",
	 const std::string &publicStr = "",
	 const std::string &description = "",
	 const std::string &contentid = "");
  ~Stream();

  const unsigned long getBitrate() const { return bitrate; }
  const std::string getName() const { return name; }
  const std::string getGenre() const { return genre; }
  const std::string getPublicStr() const { return publicStr; }
  const std::string getDescription() const { return description; }
  const std::string getContentid() const { return contentid; }
  const std::string getUrl() const { return url; }
};

typedef enum {
  XAudioLoginType = 0,
  IcyLoginType,
  FileLoginType,
  NoneLoginType
} StreamerLoginType;

class Streamer {
protected:
  int fd;
  bool bConnected;
  bool bLoggedIn;

  Stream stream;
  std::string host;
  unsigned short port;
  std::string path;
  std::string password;
  StreamerLoginType type;

  void Connect();
  void OpenFile();

  void IcyLogin();
  void XAudioLogin();
  
public:
  Streamer(const Stream &stream, const Url &url);
  Streamer(const Stream &stream, const std::string &host, unsigned short port,
	   const std::string &path, const std::string &password = "", 
	   StreamerLoginType type = NoneLoginType);
  ~Streamer();

  unsigned short getPort() const { return port; }
  std::string getHost() const { return host; }
  std::string getPath() const { return path; }
  std::string getPassword() const { return password; }
  unsigned long getBitrate() const { return stream.getBitrate(); }

  void Login();
  void Write(const unsigned char *buf, unsigned int len);
  void Read(unsigned char *buf, unsigned int len);
};


#endif /* STREAMER_H__ */
