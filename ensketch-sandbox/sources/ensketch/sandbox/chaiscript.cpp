#include <ensketch/sandbox/chaiscript.hpp>
//
#include <chaiscript/chaiscript.hpp>

namespace ensketch::sandbox::chaiscript {

// Use a module-local static instance of the ChaiScript engine.
// ChaiScript is thread-safe and provides thread-local functions and variables.
//
namespace {
ChaiScript chai{};
}  // namespace

auto eval(const filesystem::path& path) -> optional<error_type> try {
  chai.eval_file(path);
  return {};
} catch (error_type& e) {
  return e;
}

auto eval(const string& code) -> optional<error_type> try {
  chai.eval(code);
  return {};
} catch (error_type& e) {
  return e;
}

void add(value_type value, const string& name) {
  chai.add(value, name);
}

void add(function_type function, const string& name) {
  chai.add(function, name);
}

}  // namespace ensketch::sandbox::chaiscript
