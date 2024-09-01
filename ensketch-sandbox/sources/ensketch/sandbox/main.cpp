#include <ensketch/sandbox/main.hpp>
//
#include <ensketch/sandbox/console.hpp>
#include <ensketch/sandbox/frame_timer.hpp>
#include <ensketch/sandbox/log.hpp>

namespace ensketch::sandbox {

namespace detail {
namespace {
atomic<bool> done = false;
thread::id main_thread{};
frame_timer timer{10.0f};
// task_queue chaiscript_tasks{};
task_queue tasks{};
ensketch::sandbox::viewer viewer{};
}  // namespace
}  // namespace detail

static void set_main_thread() noexcept;

static void process_console_input();

static void run_main_thread();

}  // namespace ensketch::sandbox

int main(int argc, char* argv[]) {
  using namespace ensketch::sandbox;
  set_main_thread();

  // for (int i = 1; i < argc; ++i) eval_chaiscript(filesystem::path(argv[i]));
  for (int i = 1; i < argc; ++i) eval_lua(filesystem::path(argv[i]));

  jthread console_thread{[] {
    console::init();
    while (not_done()) {
      process_console_input();
    }
  }};

  jthread chaiscript_thread{[] {
    add_chaiscript_functions();
    while (not_done()) {
      while (not_done() && process_chaiscript_tasks());
      // this_thread::yield();
      this_thread::sleep_for(100ms);
    }
  }};

  jthread lua_thread{[] {
    add_lua_functions();
    while (not_done()) {
      while (not_done() && process_lua_tasks());
      this_thread::sleep_for(100ms);
    }
  }};

  run_main_thread();
}

namespace ensketch::sandbox {

static void set_main_thread() noexcept {
  detail::main_thread = this_thread::get_id();
}

auto main_thread() noexcept -> thread::id {
  return detail::main_thread;
}

void quit() noexcept {
  detail::done = true;
}

bool not_done() noexcept {
  return !detail::done.load();
}

auto main_task_queue() -> task_queue& {
  return detail::tasks;
}

auto main_viewer() noexcept -> viewer& {
  return detail::viewer;
}

void open_viewer(int width, int height) {
  auto task = detail::tasks.result_from_push([width, height] {
    detail::viewer.open(width, height);
    detail::viewer.set_vsync();
    detail::timer.set_syncing(false);
  });
  task.wait();
}

void close_viewer() {
  auto task = detail::tasks.result_from_push([] {
    detail::viewer.close();
    detail::timer.set_syncing();
  });
  task.wait();
}

static void process_console_input() {
  const auto str = console::input();
  if (!str) {
    quit();
    return;
  }
  string input = str;
  if (input.empty()) return;
  console::log("\n");
  // info(input);
  // eval_chaiscript(input);
  eval_lua(input);
}

static void run_main_thread() {
  while (not_done()) {
    detail::tasks.process();
    console::capture(format("FPS = {:6.2f}\n", detail::timer.fps()));
    console::update();
    detail::timer.update();
    if (detail::viewer) {
      detail::viewer.process_events();
      detail::viewer.update();
      detail::viewer.render();
    }
  }
  console::abort_input();
}

}  // namespace ensketch::sandbox
