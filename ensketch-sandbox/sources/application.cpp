#include <application.hpp>

application::application() {
  init_chaiscript();
}

void application::run() {
  while (running) {
    console.capture(format("FPS = {:6.2f}\n", timer.fps()));

    process_console_input();

    if (viewer) {
      viewer.process_events();
      viewer.render();
      if (!viewer.running()) {
        running = false;
      }
    }

    console.update();
    timer.update();
  }

  console.abort_input();
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
