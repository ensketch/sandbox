#include <ensketch/sandbox/console.hpp>
//
#include <replxx.hxx>

namespace ensketch::sandbox::console {

namespace {

using Replxx = replxx::Replxx;
Replxx repl{};
string base_prompt = "\x1b[1;32mensketch-sandbox\x1b[0m> ";
string prompt = "\n" + base_prompt;
string capture_str = "";

}  // namespace

void init() {
  repl.install_window_change_handler();
  repl.set_word_break_characters(" \n\t.,-%!;:=*~^'\"/?<>|[](){}");
  repl.set_indent_multiline(true);
  repl.bind_key_internal(Replxx::KEY::control(Replxx::KEY::ENTER),
                         "commit_line");
  repl.bind_key_internal(Replxx::KEY::shift(Replxx::KEY::TAB), "new_line");

  repl.bind_key_internal(Replxx::KEY::control('D'), "send_eof");
  repl.bind_key_internal(Replxx::KEY::control('C'), "abort_line");
}

auto input() -> czstring {
  return repl.input(prompt);
}

void abort_input() {
  repl.emulate_key_press(Replxx::KEY::control('C'));
  repl.emulate_key_press(Replxx::KEY::ENTER);
}

void capture(const string& str) {
  capture_str += str;
}

void update() {
  prompt = "\n" + capture_str + base_prompt;
  repl.set_prompt(prompt.c_str());
  capture_str = "";
}

void log(const string& str) {
  repl.print(str.c_str());
}

}  // namespace ensketch::sandbox::console
