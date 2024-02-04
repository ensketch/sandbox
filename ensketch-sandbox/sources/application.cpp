#include <application.hpp>

application::application() {
  init_chaiscript();
}

void application::run() {
  while (running) {
    console.capture(format("FPS = {}\n", timer.fps()));

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
