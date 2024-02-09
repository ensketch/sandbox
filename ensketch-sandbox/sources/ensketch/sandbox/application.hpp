#pragma once
#include <ensketch/sandbox/utility.hpp>
//
#include <ensketch/sandbox/console_io.hpp>
#include <ensketch/sandbox/frame_timer.hpp>
#include <ensketch/sandbox/viewer.hpp>

namespace ensketch::sandbox {

class application {
  application();

 public:
  friend auto app() noexcept -> application&;

  ~application() noexcept;

  application(const application&) = delete;
  application& operator=(const application&) = delete;
  application(application&&) = delete;
  application& operator=(application&&) = delete;

  /// This function runs the main event and update loop of the application.
  /// As of this it blocks further execution of subsequent statements.
  ///
  void run();

  ///
  ///
  void quit();

  void info(const string& str) { console.log(format("INFO:  {}\n\n", str)); }
  void warn(const string& str) { console.log(format("WARN:  {}\n\n", str)); }
  void error(const string& str) { console.log(format("ERROR: {}\n\n", str)); }

  void eval_chaiscript(const filesystem::path& script);
  void eval_chaiscript(const string& code);

  void open_viewer(int width, int height);
  void close_viewer();

 private:
  void init_chaiscript();
  void process_console_input();

 private:
  struct impl;
  impl* pimpl = nullptr;

  ensketch::sandbox::console_io console{};
  ensketch::sandbox::frame_timer timer{10.0f};
  ensketch::sandbox::viewer viewer{};
  bool running = true;
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
