#pragma once
#include <ensketch/sandbox/utility.hpp>
//
#include <ensketch/sandbox/console_io.hpp>
#include <ensketch/sandbox/frame_timer.hpp>
#include <ensketch/sandbox/viewer.hpp>

using namespace std;

class application {
 public:
  application();
  ~application() noexcept;

  void run();

  void process_console_input();

  void info(auto&&... args) {}
  void warn(auto&&... args) {}
  void error(auto&&... args) {}

  void eval_chaiscript(const filesystem::path& script);
  void eval_chaiscript(const string& code);
  void init_chaiscript();

 private:
  struct impl;
  impl* pimpl = nullptr;

  ensketch::sandbox::console_io console{};
  ensketch::sandbox::frame_timer timer{10.0f};
  ensketch::sandbox::viewer viewer{};
  bool running = true;
};
