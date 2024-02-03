#include <optional>
//
#include <format>
#include <string>
//
#include <future>
#include <thread>
//
#include <replxx.hxx>
//
#include <chaiscript/chaiscript.hpp>

using namespace std;
using czstring = const char*;

struct console_io {
  using Replxx = replxx::Replxx;

  console_io() {
    repl.install_window_change_handler();
    repl.set_word_break_characters(" \n\t.,-%!;:=*~^'\"/?<>|[](){}");
    repl.set_indent_multiline(true);
    repl.bind_key_internal(Replxx::KEY::control(Replxx::KEY::ENTER),
                           "commit_line");
    repl.bind_key_internal(Replxx::KEY::shift(Replxx::KEY::TAB), "new_line");
  }

  auto input() -> czstring { return repl.input(prompt); }

  auto async_input() -> optional<czstring> {
    if (!input_task.valid())
      input_task = async(launch::async, [this] { return input(); });
    if (future_status::ready != input_task.wait_for(0s)) return {};
    const auto result = input_task.get();
    input_task = {};
    return result;
  }

  void log(const string& str) { repl.print(str.c_str()); }

  void capture(const string& str) { capture_str += str; }

  void update() {
    prompt = "\n" + capture_str + base_prompt;
    repl.set_prompt(prompt.c_str());
    capture_str = "";
  }

  Replxx repl{};
  string base_prompt = "\x1b[1;32mensketch-sandbox\x1b[0m> ";
  string prompt = "\n" + base_prompt;
  string capture_str = "";

  future<czstring> input_task{};
};

int main(int argc, char* argv[]) {
  console_io console{};
  int i = 0;

  chaiscript::ChaiScript chai{};
  const auto eval_chaiscript = [&](const string& code) {
    try {
      chai.eval(code);
    } catch (chaiscript::exception::eval_error& e) {
      console.log(format("ERROR: ChaiScript String Eval:\n{}\n", e.what()));
    }
  };

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
    this_thread::sleep_for(100ms);
  }
}
