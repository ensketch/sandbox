#include <ensketch/sandbox/main.hpp>
//
#include <ensketch/sandbox/chaiscript.hpp>
#include <ensketch/sandbox/log.hpp>
#include <ensketch/sandbox/task_queue.hpp>

namespace ensketch::sandbox {

namespace detail {
namespace {
task_queue tasks{};
}  // namespace
}  // namespace detail

bool process_chaiscript_tasks() {
  return detail::tasks.process();
}

void eval_chaiscript(const string& code) {
  detail::tasks.push([code] {
    auto error = chaiscript::eval(code);
    if (!error) return;
    log::error(format("ChaiScript String Eval: {}", error->what()));
  });
}

void eval_chaiscript(const filesystem::path& path) {
  detail::tasks.push([path] {
    // const auto abs_path = absolute(path);
    // lookup_path = abs_path.parent_path();
    // auto error = chaiscript::eval(abs_path);
    // lookup_path = filesystem::path{};
    auto error = chaiscript::eval(path);
    if (!error) return;
    log::error(format("ChaiScript File Eval: {}", error->what()));
  });
}

}  // namespace ensketch::sandbox
