#pragma once
#include <ensketch/sandbox/utility.hpp>
//
#include <replxx.hxx>

namespace ensketch::sandbox {

struct console_io {
  using Replxx = replxx::Replxx;

  console_io() {
    repl.install_window_change_handler();
    repl.set_word_break_characters(" \n\t.,-%!;:=*~^'\"/?<>|[](){}");
    repl.set_indent_multiline(true);
    repl.bind_key_internal(Replxx::KEY::control(Replxx::KEY::ENTER),
                           "commit_line");
    repl.bind_key_internal(Replxx::KEY::shift(Replxx::KEY::TAB), "new_line");

    repl.bind_key_internal(Replxx::KEY::control('D'), "send_eof");
    repl.bind_key_internal(Replxx::KEY::control('C'), "abort_line");
  }

  void close() {
    repl.emulate_key_press(Replxx::KEY::control('C'));
    repl.emulate_key_press(Replxx::KEY::control('D'));
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

  void abort_input() {
    repl.emulate_key_press(Replxx::KEY::control('C'));
    repl.emulate_key_press(Replxx::KEY::ENTER);
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

}  // namespace ensketch::sandbox
