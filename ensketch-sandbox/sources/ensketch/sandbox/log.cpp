#include <ensketch/sandbox/log.hpp>
//
#include <ensketch/luarepl/luarepl.hpp>

namespace ensketch::sandbox::log {

static void log(auto&& prefix, auto&& str, source_location location) {
  // console::log(str);
  luarepl::log(fmt::format("{}{}\n\n\t{}\n\n",
                           forward<decltype(prefix)>(prefix),
                           string_from(location), forward<decltype(str)>(str)));
}

auto string_from(source_location location) -> string {
  return fmt::format(
      fg(color::gray), "{}\n\t{}:",
      fmt::format("{}:{}:{}: ",  //
                  relative(filesystem::path(location.file_name())).string(),
                  location.line(), location.column()),
      location.function_name());
}

void debug(const string& str, source_location location) {
  // console::log(fmt::format("DEBUG: {}\n\n\t{}\n", string_from(location), str));
  log(fmt::format(fg(color::gray), "DEBUG:   "), str, location);
}

void info(const string& str, source_location location) {
  // console::log(fmt::format("INFO:  {}\n\n\t{}\n", string_from(location), str));
  // console::log(fmt::format("INFO:  {}\n", str));
  log(fmt::format(fg(color::green), "INFO:    "), str, location);
}

void warn(const string& str, source_location location) {
  // console::log(fmt::format("WARN:  {}\n\n\t{}\n", string_from(location), str));
  // console::log(fmt::format("WARN:  {}\n", str));
  log(fmt::format(fg(color::orange), "WARNING: "), str, location);
}

void error(const string& str, source_location location) {
  // console::log(fmt::format("ERROR: {}\n\n\t{}\n", string_from(location), str));
  // console::log(fmt::format("ERROR: {}\n", str));
  log(fmt::format(fg(color::red), "ERROR:   "), str, location);
}

}  // namespace ensketch::sandbox::log
