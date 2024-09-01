#pragma once
#include <ensketch/sandbox/utility.hpp>
//
#include <sol/sol.hpp>

namespace ensketch::sandbox::lua {

extern sol::state state;

auto eval(const filesystem::path& script) -> optional<sol::error>;
auto eval(const string& code) -> optional<sol::error>;

constexpr void add(const string& name, auto&& f) {
  state.set_function(name, std::forward<decltype(f)>(f));
}

}  // namespace ensketch::sandbox::lua
