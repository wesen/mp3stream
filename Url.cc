/*
 * mp3stream - encode /dev/dsp and send it to a shoutcast server
 * 2005 - bl0rg.net - public domain
 *
 * Url.cc - url implementation, a very dumb url parser.
 */

using namespace std;

#include "Url.h"

Url::Url(const std::string &url) :
  protocol(""), login(""), password(""), host(""), port(0), path("") {

  string::size_type strStart = 0;

  /* protocol */
  string::size_type protoEnd = url.find("://", strStart);
  if (protoEnd == string::npos)
    throw UrlError("Could not parse " + url);
  protocol = url.substr(strStart, protoEnd - strStart);
  strStart = protoEnd + 3;

  /* location */
  string::size_type locationEnd = url.find("/", strStart);
  if (locationEnd == string::npos)
    locationEnd = url.length();

  string location = url.substr(strStart, locationEnd - strStart);
  
  /* authentication */
  strStart = 0;
  string::size_type authEnd = location.find("@", strStart);
  if (authEnd != string::npos) {
    parseAuthString(location.substr(strStart, authEnd - strStart));
    strStart = authEnd + 1;
  }
  parseLocationString(location.substr(strStart));

  path = url.substr(locationEnd);
}

Url::~Url() { }

void Url::parseAuthString(const std::string &str) {
  string::size_type loginEnd = str.find(":");
  if (loginEnd != string::npos) {
    login = str.substr(0, loginEnd);
    password = str.substr(loginEnd + 1);
  } else {
    login = str;
  }
}

void Url::parseLocationString(const std::string &str) {
  string::size_type hostEnd = str.find(":");
  if (hostEnd != string::npos) {
    host = str.substr(0, hostEnd);
    string portStr = str.substr(hostEnd + 1);
    port = atoi(portStr.c_str());
  } else {
    host = str;
  }
}

