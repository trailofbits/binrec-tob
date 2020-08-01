#include <sstream>
#include <string>

inline std::string intToString(unsigned int num) {
  std::ostringstream s;
  s << std::hex << num;
  return s.str();
}
