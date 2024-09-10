#include <ensketch/sandbox/application.hpp>

namespace ensketch::sandbox {

auto app() noexcept -> application& {
  static application _app{};
  return _app;
}

auto application::main_thread() const noexcept -> thread::id {
  // This is read-only access and, thus,
  // doesn't need any thread synchronization.
  //
  return main_tid;
}

void application::run() {
  // If 'run' has not been called so far
  // then set the current thread id as the main thread.
  {
    // Can probably be done lock-free by using the 'running' variable.
    scoped_lock lock{run_mutex};
    if (main_tid != thread::id{}) return;
    main_tid = this_thread::get_id();
  }
  running = true;

  // chaiscript_run();

  while (running) {
    console.capture(format("FPS = {:6.2f}\n", timer.fps()));

    process_console_input();

    tasks.process();

    if (viewer) {
      viewer.process_events();
      viewer.update();
      viewer.render();
    }

    console.update();
    timer.update();
  }

  console.abort_input();

  running = false;
  main_tid = {};
}

void application::quit() {
  running = false;
}

void application::process_console_input() {
  if (const auto ready = console.async_input()) {
    const auto tmp = ready.value();
    if (!tmp) {
      running = false;
      return;
    }
    string input = tmp;
    if (input.empty()) return;
    console.log("\n");
    // chaiscript_eval(input);
  }
}

void application::basic_open_viewer(int width, int height) {
  viewer.open(width, height);
  viewer.set_vsync();
  timer.set_syncing(false);
}

auto application::async_open_viewer(int width, int height) -> future<void> {
  return tasks.push(
      [this, width, height] { basic_open_viewer(width, height); });
}

void application::open_viewer(int width, int height) {
  if (this_thread::get_id() == main_thread()) {
    basic_open_viewer(width, height);
    return;
  }
  auto task = async_open_viewer(width, height);
  task.wait();
}

void application::basic_close_viewer() {
  viewer.close();
  timer.set_syncing();
}

auto application::async_close_viewer() -> future<void> {
  return tasks.push([this] { basic_close_viewer(); });
}

void application::close_viewer() {
  if (this_thread::get_id() == main_thread()) {
    basic_close_viewer();
    return;
  }
  auto task = async_close_viewer();
  task.wait();
}

}  // namespace ensketch::sandbox
