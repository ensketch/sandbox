#include <ensketch/sandbox/application.hpp>

namespace ensketch::sandbox {

auto app() noexcept -> application& {
  static application _app{};
  return _app;
}

application::application() {
  init_chaiscript();
  info("Successfully constructed application\n");
}

void application::run() {
  running = true;

  while (running) {
    console.capture(format("FPS = {:6.2f}\n", timer.fps()));

    process_console_input();

    if (viewer) {
      viewer.process_events();
      viewer.update();
      viewer.render();
    }

    console.update();
    timer.update();
  }

  console.abort_input();
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

void application::open_viewer(int width, int height) {
  viewer.open(width, height);
  viewer.set_vsync();
  timer.set_syncing(false);
}

void application::close_viewer() {
  viewer.close();
  timer.set_syncing();
}

}  // namespace ensketch::sandbox
