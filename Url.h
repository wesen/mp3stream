#ifndef URL_H__
#define URL_H__

#include <string>

#include "Error.h"

class UrlError : public Error {
public:
  UrlError(std::string msg) : Error(msg) { }
  virtual ~UrlError() { }
};


class Url {
protected:
  std::string protocol;
  std::string login;
  std::string password;
  std::string host;
  unsigned short port;
  std::string path;

protected:
  void parseAuthString(const std::string &str);
  void parseLocationString(const std::string &str);
  
public:
  Url(const std::string &string);
  ~Url();

  std::string getProtocol() const { return protocol; }
  std::string getLogin() const { return login; }
  std::string getPassword() const { return password; }
  std::string getHost() const { return host; }
  unsigned short getPort() const { return port; }
  std::string getPath() const { return path; }

};

#endif /* URL_H__ */

