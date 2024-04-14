#pragma once
#include <ensketch/sandbox/utility.hpp>

namespace ensketch::sandbox {

class task_queue {
 public:
  using task_prototype = void();
  using task_type = move_only_function<task_prototype>;
  using queue_type = queue<task_type>;

  bool process();
  void process_all();

  void push(auto&& f) {
    scoped_lock lock{mutex};
    tasks.emplace(forward<decltype(f)>(f));
  }

  [[nodiscard]] auto result_from_push(auto&& f) {
    packaged_task task{forward<decltype(f)>(f)};
    auto result = task.get_future();
    push(move(task));
    return result;
  }

 private:
  queue_type tasks{};
  mutable std::mutex mutex{};
};

}  // namespace ensketch::sandbox
