#include <ensketch/sandbox/application.hpp>

namespace ensketch::sandbox {

auto app() noexcept -> application& {
  static application _app{};
  return _app;
}

application::application() {
  init_chaiscript();
  debug("Successfully constructed application.");
}

void application::run() {
  {
    scoped_lock lock{run_mutex};
    if (run_thread != thread::id{})
      throw runtime_error("Failed to run application! It is already running.");
    run_thread = this_thread::get_id();
  }

  running = true;

  while (running) {
    console.capture(format("FPS = {:6.2f}\n", timer.fps()));

    process_console_input();
    handle_eval_chaiscript_task();

    process_task_queue();

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
  run_thread = {};
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
    eval_chaiscript(input);
  }
}

void application::async_eval_chaiscript(const filesystem::path& path) {
  if (eval_chaiscript_task.valid()) {
    error(format(
        "Failed to start asynchronous evaluation of ChaiScript file.\n Another "
        "file is currently evaluated.\nfile='{}'",
        path.string()));
    return;
  }

  eval_chaiscript_task = async(
      launch::async,
      [this](const auto& path) {
        init_chaiscript();
        eval_chaiscript(path);
      },
      path);
  info(
      format("Started asynchronous evaluation of ChaiScript file.\nfile = '{}'",
             path.string()));
}

void application::handle_eval_chaiscript_task() {
  if (!eval_chaiscript_task.valid()) return;
  if (future_status::ready != eval_chaiscript_task.wait_for(0s)) return;
  eval_chaiscript_task = {};
  info("Sucessfully finished asynchronous evaluation of ChaiScript file.");
}

void application::process_task_queue() {
  packaged_task<void()> task{};
  {
    scoped_lock lock{task_queue_mutex};
    if (task_queue.empty()) return;
    task = move(task_queue.front());
    task_queue.pop();
  }
  task();
}

void application::basic_open_viewer(int width, int height) {
  viewer.open(width, height);
  viewer.set_vsync();
  timer.set_syncing(false);
}

auto application::async_open_viewer(int width, int height) -> future<void> {
  return future_from_task_queue(
      [this, width, height] { basic_open_viewer(width, height); });
}

void application::open_viewer(int width, int height) {
  if (this_thread::get_id() == running_thread()) {
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
  return future_from_task_queue([this] { basic_close_viewer(); });
}

void application::close_viewer() {
  if (this_thread::get_id() == running_thread()) {
    basic_close_viewer();
    return;
  }
  auto task = async_close_viewer();
  task.wait();
}

}  // namespace ensketch::sandbox
