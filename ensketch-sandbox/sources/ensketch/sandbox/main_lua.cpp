#include <ensketch/sandbox/main.hpp>
//
#include <ensketch/sandbox/log.hpp>
#include <ensketch/sandbox/lua.hpp>
#include <ensketch/sandbox/task_queue.hpp>

namespace ensketch::sandbox {

namespace detail {
namespace {
task_queue tasks{};
}  // namespace
}  // namespace detail

bool process_lua_tasks() {
  return detail::tasks.process();
}

void eval_lua(const string& code) {
  detail::tasks.push([code] {
    auto error = lua::eval(code);
    if (!error) return;
    log::error(format("Lua String Eval: {}", error->what()));
  });
}

void eval_lua(const filesystem::path& path) {
  detail::tasks.push([path] {
    // const auto abs_path = absolute(path);
    // lookup_path = abs_path.parent_path();
    // auto error = lua::eval(abs_path);
    // lookup_path = filesystem::path{};
    auto error = lua::eval(path);
    if (!error) return;
    log::error(format("Lua File Eval: {}", error->what()));
  });
}

}  // namespace ensketch::sandbox
