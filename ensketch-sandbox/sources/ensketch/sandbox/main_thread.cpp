#include <ensketch/sandbox/main_thread.hpp>

namespace ensketch::sandbox::main_thread {

// The main thread id is stored as global, static, and constant
// `std::thread::id` object that can only be accessed in this source unit.
// To retrieve its value, `main_thread::id()` must be called.
// To automatically receive the main thread id without extra function calls,
// the static variable is initialized by a call to `std::this_thread::get_id()`.
// Static initialization should be carried out by the
// main thread and, consequently, provide the correct id.
// Not all platforms might behave in the same way
// and this procedure might not work for libraries.
// Further research needs to be done.
static const std::thread::id _id = std::this_thread::get_id();

task_queue tasks{};

auto id() noexcept -> std::thread::id {
  return _id;
}

void run(std::stop_token stop_token) {
  // This function must be called by the main thread.
  if (id() != std::this_thread::get_id())
    throw std::runtime_error(
        "Failed to run main thread as it was invoked on a different thread.");
  tasks.run(stop_token);
}

}  // namespace ensketch::sandbox::main_thread
