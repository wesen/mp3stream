#include <string>
#include <iostream>
using namespace std;

#include "Url.h"

void printUrl(const Url &url) {
  cout << url.getProtocol() << "://";
  string login = url.getLogin();
  string password = url.getPassword();
  string host = url.getHost();
  unsigned short port = url.getPort();

  if (login != "") {
    cout << login;
    if (password != "") {
      cout << ":" << password;
    }
    cout << "@";
  }
  cout << host;
  if (port != 0) {
    cout << ":" << port;
  }
  cout << "/" << url.getPath();
  cout << endl;
}

int main(void) {
  Url url("file:///foobar/");
  printUrl(url);

  printUrl(Url("icy://localhost:8000/mp3test"));
  printUrl(Url("icy://manuel@localhost:8000/mp3test"));
  printUrl(Url(""));

  return 0;
}
