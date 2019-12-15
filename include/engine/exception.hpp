#ifndef MIX_EXCEPTION_HPP
#define MIX_EXCEPTION_HPP
#include <stdexcept>
#include <string>

class EngineException : public std::runtime_error {
  char m[80]; 
  const char* file;
  public:
    EngineException(const char* msg, const char* _file) : std::runtime_error(msg), file(_file) {
      std::sprintf(m, "in file %s:\n%s", _file, msg);
    }
    const char* what() throw() {
      return m;
    }
};
#endif
