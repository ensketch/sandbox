#include <ensketch/sandbox/task_queue.hpp>

namespace ensketch::sandbox {

bool task_queue::process() {
  task_type task{};
  {
    scoped_lock lock{mutex};
    if (tasks.empty()) return false;
    task = move(tasks.front());
    tasks.pop();
  }
  task();
  return true;
}

void task_queue::process_all() {
  while (process())
    ;
}

}  // namespace ensketch::sandbox
