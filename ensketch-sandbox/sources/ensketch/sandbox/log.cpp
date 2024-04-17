#include <ensketch/sandbox/log.hpp>
//
#include <ensketch/sandbox/console.hpp>

namespace ensketch::sandbox::log {

static void log(const string& str) {
  console::log(str);
}

void debug(const string& str) {
  log(format("DEBUG: {}\n", str));
}

void info(const string& str) {
  log(format("INFO:  {}\n", str));
}

void warn(const string& str) {
  log(format("WARN:  {}\n", str));
}

void error(const string& str) {
  log(format("ERROR: {}\n", str));
}

}  // namespace ensketch::sandbox::log
