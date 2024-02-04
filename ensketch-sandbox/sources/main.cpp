#include <chaiscript/chaiscript.hpp>
//
#include <console_io.hpp>
#include <viewer.hpp>

using namespace std;
using czstring = const char*;

int main(int argc, char* argv[]) {
  chaiscript::ChaiScript chai{};
  ensketch::sandbox::console_io console{};
  int i = 0;

  auto viewer = make_unique<ensketch::sandbox::viewer>(500, 500);

  const auto eval_chaiscript = [&](const string& code) {
    try {
      chai.eval(code);
    } catch (chaiscript::exception::eval_error& e) {
      console.log(format("ERROR: ChaiScript String Eval:\n{}\n", e.what()));
    }
  };

  chai.add(chaiscript::fun([&](int width, int height) {
             viewer = make_unique<ensketch::sandbox::viewer>(width, height);
           }),
           "reset");

  while (true) {
    i = (i + 1) % 4;
    console.capture(format("i = {}\n", i));

    if (const auto ready = console.async_input()) {
      const auto tmp = ready.value();
      if (!tmp) break;
      string input = tmp;
      if (input.empty()) continue;
      console.log("\n");
      eval_chaiscript(input);
    }

    console.update();
    // this_thread::sleep_for(100ms);

    viewer->process_events();
    viewer->render();
  }
}
