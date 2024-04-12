#pragma once
#include <ensketch/sandbox/utility.hpp>
//
// #include <chaiscript/chaiscript_basic.hpp>
#include <chaiscript/chaiscript_defines.hpp>
#include <chaiscript/dispatchkit/boxed_number.hpp>
#include <chaiscript/dispatchkit/dispatchkit.hpp>
#include <chaiscript/dispatchkit/dynamic_object.hpp>
#include <chaiscript/dispatchkit/function_call.hpp>
#include <chaiscript/language/chaiscript_eval.hpp>

namespace ensketch::sandbox {

class interpreter {
 public:
  using chaiscript_value_type = chaiscript::Boxed_Value;
  using chaiscript_function_type = chaiscript::Proxy_Function;

  void run();
  auto running_thread() const noexcept -> thread::id { return run_thread; }

  void quit();

  auto future_from_task_queue(auto&& f) {
    packaged_task<void()> task{forward<decltype(f)>(f)};
    auto result = task.get_future();
    scoped_lock lock{task_queue_mutex};
    task_queue.push(move(task));
    return result;
  }

  void eval_chaiscript(const filesystem::path& script) const;
  void eval_chaiscript(const string& code) const;

  void add(chaiscript_value_type value, const string& name);
  void add(chaiscript_function_type function, const string& name);

  void add(auto&& f, const string& name) {
    add(chaiscript::fun(forward<decltype(f)>(f)), name);
  }

 private:
  void process_task_queue();

 private:
  thread::id run_thread{};
  mutex run_mutex{};
  bool running = false;

  mutex task_queue_mutex{};
  queue<packaged_task<void()>> task_queue{};
};

}  // namespace ensketch::sandbox
