#include <chaiscript/chaiscript.hpp>
//
#include <ensketch/sandbox/console_io.hpp>
#include <ensketch/sandbox/frame_timer.hpp>
#include <ensketch/sandbox/viewer.hpp>

using namespace std;
using czstring = const char*;

int main(int argc, char* argv[]) {
  chaiscript::ChaiScript chai{};
  ensketch::sandbox::console_io console{};
  ensketch::sandbox::frame_timer timer{10.0f};
  ensketch::sandbox::viewer viewer{};
  bool running = true;

  const auto eval_chaiscript = [&](const string& code) {
    try {
      chai.eval(code);
    } catch (chaiscript::exception::eval_error& e) {
      console.log(format("ERROR: ChaiScript String Eval:\n{}\n", e.what()));
    }
  };

  const auto process_input = [&] {
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
  };

  chai.add(chaiscript::fun([&](int width, int height) {
             viewer.open(width, height);
             viewer.set_vsync();
             timer.set_syncing(false);
           }),
           "viewer_open");

  chai.add(chaiscript::fun([&]() {
             viewer.close();
             timer.set_syncing();
           }),
           "viewer_close");

  chai.add(
      chaiscript::fun([](int major, int minor) {
        ensketch::sandbox::viewer::opengl_context_settings.majorVersion = major;
        ensketch::sandbox::viewer::opengl_context_settings.minorVersion = minor;
      }),
      "set_opengl_version");

  while (running) {
    console.capture(format("FPS = {}\n", timer.fps()));

    process_input();

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
