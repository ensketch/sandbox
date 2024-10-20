#pragma once
#include <ensketch/sandbox/utility.hpp>
//
#include <source_location>
//
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
// #include <fmt/std.h>

namespace ensketch::sandbox::log {

using namespace fmt;
using namespace fmt::literals;
using fmt::format;

auto string_from(source_location location) -> string;

void text(std::string_view str);
void debug(const string& str, source_location = source_location::current());
void info(const string& str, source_location = source_location::current());
void warn(const string& str, source_location = source_location::current());
void error(const string& str, source_location = source_location::current());

}  // namespace ensketch::sandbox::log
