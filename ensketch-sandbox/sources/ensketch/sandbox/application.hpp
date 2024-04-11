#pragma once
#include <ensketch/sandbox/utility.hpp>
//
#include <ensketch/sandbox/console_io.hpp>
#include <ensketch/sandbox/frame_timer.hpp>
#include <ensketch/sandbox/interpreter.hpp>
#include <ensketch/sandbox/viewer.hpp>

namespace ensketch::sandbox {

class application {
  // The application represents a singleton.
  // Hence, its constructor is private and
  // not allowed to be called from outside.
  //
  application();

 public:
  // As the constructor is private, there must be a function that
  // calls it to handle the construction and handling of the application.
  // This function returns a reference to the application.
  //
  friend auto app() noexcept -> application&;

  //
  ~application() noexcept;

  // The application is not allowed to be copied or moved.
  //
  application(const application&) = delete;
  application& operator=(const application&) = delete;
  application(application&&) = delete;
  application& operator=(application&&) = delete;

  /// Standard Logging Functions
  ///
  void debug(const string& str) const {
    console.log(format("DEBUG: {}\n\n", str));
  }
  void info(const string& str) const {
    console.log(format("INFO:  {}\n\n", str));
  }
  void warn(const string& str) const {
    console.log(format("WARN:  {}\n\n", str));
  }
  void error(const string& str) const {
    console.log(format("ERROR: {}\n\n", str));
  }

  /// This function runs the main event and update loop of the application.
  /// As of this it blocks further execution of subsequent statements.
  ///
  void run();

  /// This functions returns the thread id on which 'run' has been called.
  ///
  auto running_thread() const noexcept -> thread::id { return run_thread; }

  ///
  ///
  void quit();

  void async_eval_chaiscript(const filesystem::path& script);

  auto future_from_task_queue(auto&& f) {
    packaged_task<void()> task{forward<decltype(f)>(f)};
    auto result = task.get_future();
    scoped_lock lock{task_queue_mutex};
    task_queue.push(move(task));
    return result;
  }

  void open_viewer(int width, int height);
  void close_viewer();
  auto async_open_viewer(int width, int height) -> future<void>;
  auto async_close_viewer() -> future<void>;

 private:
  void init_interpreter_module();
  void process_console_input();
  void process_task_queue();
  void process_eval_chaiscript_task();

  void basic_open_viewer(int width, int height);
  void basic_close_viewer();

 private:
  bool running = true;
  thread::id run_thread{};
  mutex run_mutex{};

  ensketch::sandbox::console_io console{};
  ensketch::sandbox::frame_timer timer{10.0f};
  ensketch::sandbox::viewer viewer{};

  ensketch::sandbox::interpreter interpreter{};
  future<void> interpreter_task{};
  future<void> eval_chaiscript_task{};

  mutex task_queue_mutex{};
  queue<packaged_task<void()>> task_queue{};
};

// If a friend declaration in a non-local class first
// declares a class or function the friend class or function
// is a member of the innermost enclosing namespace.
// The name of the friend is not found by unqualified lookup
// or by qualified lookup until a matching declaration
// is provided in that namespace scope.
//
auto app() noexcept -> application&;

}  // namespace ensketch::sandbox
