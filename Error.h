#ifndef ERROR_H__
#define ERROR_H__

#include <string>

class Error {
protected:
  std::string errorMessage;

public:
  Error(std::string msg) : errorMessage(msg) { }
  virtual ~Error() { }

  std::string getMessage() { return errorMessage; }
};

#endif // ERROR_H__
