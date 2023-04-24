#include "utils.hpp"
#include <sstream>

bool ws_strtoi(int *dest, const std::string src) {
  for (std::string::const_iterator it = src.begin(); it != src.end(); it++) {
    if (!isdigit(*it)) {
      return false;
    }
  }
  std::stringstream ss(src);
  ss >> *dest;
  return !ss.fail();
}
