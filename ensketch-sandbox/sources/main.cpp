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
  int i = 0;

  unique_ptr<ensketch::sandbox::viewer> viewer{};
  ensketch::sandbox::frame_timer timer{10.0f};

  const auto eval_chaiscript = [&](const string& code) {
    try {
      chai.eval(code);
    } catch (chaiscript::exception::eval_error& e) {
      console.log(format("ERROR: ChaiScript String Eval:\n{}\n", e.what()));
    }
  };

  chai.add(chaiscript::fun([&](int width, int height) {
             viewer = make_unique<ensketch::sandbox::viewer>(width, height);
             viewer->set_vsync();
             timer.set_syncing(false);
           }),
           "viewer_open");
  chai.add(chaiscript::fun([&]() {
             viewer = nullptr;
             timer.set_syncing();
           }),
           "viewer_close");

  while (true) {
    // console.capture(format("idle time = {}s\n", idle_time));
    // console.capture(format("frame time = {}s\n", current_frame_time));
    console.capture(format("FPS = {}\n", timer.fps()));

    if (const auto ready = console.async_input()) {
      const auto tmp = ready.value();
      if (!tmp) break;
      string input = tmp;
      if (input.empty()) continue;
      console.log("\n");
      eval_chaiscript(input);
    }

    if (viewer) {
      viewer->process_events();
      viewer->render();
    }

    console.update();
    timer.update();
  }
}
