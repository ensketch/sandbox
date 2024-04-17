#pragma once
#include <ensketch/sandbox/utility.hpp>

namespace ensketch::sandbox::log {

void debug(const string& str);
void info(const string& str);
void warn(const string& str);
void error(const string& str);

}  // namespace ensketch::sandbox::log
