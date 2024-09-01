#include <ensketch/sandbox/lua.hpp>
//

namespace ensketch::sandbox::lua {

sol::state state{};

auto eval(const filesystem::path& path) -> optional<sol::error> {
  auto result = state.safe_script_file(path, sol::script_pass_on_error);
  if (!result.valid()) return sol::error{result};
  return {};
}

auto eval(const string& code) -> optional<sol::error> {
  auto result = state.safe_script(code, sol::script_pass_on_error);
  if (!result.valid()) return sol::error{result};
  return {};
}

}  // namespace ensketch::sandbox::lua
