#include <ensketch/sandbox/application.hpp>

namespace ensketch::sandbox {

auto app() noexcept -> application& {
  static application _app{};
  return _app;
}

void application::run() {
  {
    scoped_lock lock{run_mutex};
    if (run_thread != thread::id{})
      throw runtime_error("Failed to run application! It is already running.");
    run_thread = this_thread::get_id();
  }
  running = true;

  // interpreter_task = async(launch::async, [this] {
  //   init_interpreter_module();
  //   interpreter.run();
  // });

  chaiscript_run();

  while (running) {
    console.capture(format("FPS = {:6.2f}\n", timer.fps()));

    process_console_input();
    // process_eval_chaiscript_task();

    // process_task_queue();
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
  // interpreter.quit();

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
    // eval_chaiscript(input);
    // auto task = interpreter.future_from_task_queue(
    //     [this, input] { interpreter.eval_chaiscript(input); });
    // chaiscript_tasks.push([this, input] { chaiscript::eval(input); });
    chaiscript_eval(input);
  }
}

void application::basic_open_viewer(int width, int height) {
  viewer.open(width, height);
  viewer.set_vsync();
  timer.set_syncing(false);
}

auto application::async_open_viewer(int width, int height) -> future<void> {
  return tasks.result_from_push(
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
  return tasks.result_from_push([this] { basic_close_viewer(); });
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
