#include "luarepl.hpp"
//
#include "lexer.hpp"
//
#include <replxx.hxx>
//
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace ensketch::luarepl {

// Type Aliases
//
using repl_state = replxx::Replxx;
using completions_t = repl_state::completions_t;
using KEY = repl_state::KEY;
using ACTION_RESULT = repl_state::ACTION_RESULT;
using Color = repl_state::Color;

// Global Variables
//
namespace {
std::atomic<bool> is_done{false};
std::atomic<bool> is_interrupted{false};
std::atomic<bool> is_evaluating{false};
repl_state repl{};
std::mutex prompt_mutex{};
std::string prompt_str{};
task_queue tasks{};
std::chrono::time_point<std::chrono::high_resolution_clock> eval_start{};
std::chrono::duration<float32> _last_eval_time{};
}  // namespace
std::mutex mutex{};
sol::state lua{};
std::function<bool()> done{[] { return is_done.load(); }};
std::function<void()> quit{[] { is_done = true; }};

/// Attempt to abort the blocking function call `repl.input(...)`.
/// This is done by emulating the escape key press.
/// The escape key must bind to the bail action to make it work.
///
static void abort_input() {
  repl.emulate_key_press(KEY::ESCAPE);
}

/// After receiving an input string from `repl.input`, process it.
/// `nullptr` will lead to termination of the application.
/// Empty strings will interrupt the previous running code.
/// All other inputs will be fed to the Lua state.
///
static void process_input(czstring zstr) {
  if (!zstr) {
    quit();
    is_interrupted = true;
    return;
  }
  auto str = string_view{zstr};
  if (str.empty()) {
    is_interrupted = true;
    return;
  }
  repl.history_add(zstr);
  async_run(str);
}

/// Call `repl.input(...)` by wrapping it using `async` to make it
/// interruptible. The function also handles the prompt and prompt animations.
///
static void process_repl() {
  auto task = async_invoke([] { return repl.input(prompt()); });
  size_t i = 0;
  const auto start = clock::now();
  while (task.wait_for(10ms) != std::future_status::ready) {
    if (not done()) continue;
    abort_input();
    return;
  }
  process_input(task.get());
}

/// The syntax highlighting callback to properly color the Lua code.
/// This code is run each time the command-line string is modified.
/// For coloring, a Lua lexer/parser is used to also indicate errors.
/// Additionally, keys like `enter` may rebind to improve editing intuition.
///
static void highlighter(const std::string& context,
                        repl_state::colors_t& colors) {
  const auto base = context.c_str();
  const auto ctx = lexer::scan(context.c_str());

  const auto set_color_for_token = [&](auto& t, Color color) {
    const auto offset = t.source().begin() - base;
    for (size_t i = 0; i < t.source().size(); ++i)
      colors.at(offset + i) = color;
  };

  for (auto& t : ctx.tokens)
    t | match{
            [&](auto t) { /* nothing */ },
            [&](numeral t) { set_color_for_token(t, Color::BROWN); },
            [&](string_literal t) { set_color_for_token(t, Color::GREEN); },
            [&](control_keyword auto t) {
              set_color_for_token(t, Color::BLUE);
            },
            [&](operator_keyword auto t) {
              set_color_for_token(t, Color::CYAN);
            },
            [&](operator_or_punctuation auto t) {
              set_color_for_token(t, Color::CYAN);
            },
            [&](value_keyword auto t) { set_color_for_token(t, Color::BROWN); },
        };

  // Do not use the actual luarepl state to circumvent deadlock.
  if (sol::state{}.load(context).status() == sol::load_status::syntax)
    repl.bind_key_internal(KEY::ENTER, "new_line");
  else
    repl.bind_key_internal(KEY::ENTER, "commit_line");
}

static auto completion(const std::string& input, int& length) -> completions_t {
  completions_t completions;
  completions.push_back("print");
  completions.push_back("for");
  return completions;
}

/// Initialize the global Lua state of the REPL.
///
static void init_lua() {
  // Load all useful standard libraries.
  lua.open_libraries(  //
      sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string,
      sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::debug,
      sol::lib::bit32, sol::lib::io);
  // Set debug hook every one million instructions to allow for interrupts.
  lua_sethook(
      lua,
      [](lua_State* L, lua_Debug*) {
        if (!is_interrupted) return;
        luaL_error(L, "interrupted");
      },
      LUA_MASKCOUNT, 1'000'000);
}

/// Initialize the global REPL state.
///
static void init_repl() {
  repl.install_window_change_handler();
  repl.set_indent_multiline(true);
  repl.set_word_break_characters(" \n\t.,-%!;:=*~^'\"/?<>|[](){}");
  repl.bind_key_internal(KEY::control('D'), "send_eof");
  repl.bind_key_internal(KEY::control('C'), "abort_line");
  repl.bind_key(KEY::ESCAPE, [](char32_t code) { return ACTION_RESULT::BAIL; });

  {
    std::ifstream history{"history.txt"};
    repl.history_load(history);
  }
  repl.set_max_history_size(128);

  repl.set_highlighter_callback(highlighter);
  repl.set_completion_callback(completion);
}

auto prompt() -> std::string {
  std::scoped_lock lock{prompt_mutex};
  return prompt_str;
}

void set_prompt(std::string_view str) {
  std::scoped_lock lock{prompt_mutex};
  prompt_str = str;
  repl.set_prompt(prompt_str);
}

void log(czstring str) {
  repl.print("%s\n", str);
}

void log(std::string_view str) {
  log(std::string{str}.c_str());
}

void run() {
  init_lua();
  init_repl();
  auto lua_task = async_invoke([] {
    while (not done()) {
      while (tasks.process())
        if (done()) return;
      std::this_thread::sleep_for(100ms);
    }
  });
  while (not done()) process_repl();
  {
    std::ofstream history{"history.txt"};
    repl.history_save(history);
  }
  lua_task.get();
}

auto eval_file(const std::filesystem::path& path)
    -> sol::protected_function_result {
  std::scoped_lock lock{mutex};
  is_interrupted = false;
  is_evaluating = true;
  auto result = lua.safe_script_file(path, sol::script_pass_on_error);
  is_evaluating = false;
  return result;
}

auto eval(std::string_view str) -> sol::protected_function_result {
  std::scoped_lock lock{mutex};
  is_interrupted = false;
  is_evaluating = true;
  eval_start = std::chrono::high_resolution_clock::now();
  auto result = lua.safe_script(str, sol::script_pass_on_error);
  _last_eval_time = std::chrono::duration<float32>(
      std::chrono::high_resolution_clock::now() - eval_start);
  is_evaluating = false;
  return result;
}

void async_run_file(const std::filesystem::path& path) {
  tasks.push_and_discard([path] {
    auto result = eval_file(path);
    if (result.valid()) return;
    log(sol::error{result}.what());
  });
}

void async_run(std::string&& str) {
  tasks.push_and_discard([str = std::move(str)] {
    auto result = eval(str);
    if (result.valid()) return;
    log(sol::error{result}.what());
  });
}

void async_run(std::string_view str) {
  // We need to copy the content of `str` to ensure the task won't
  // access the view's content after the end of its lifetime.
  async_run(string{str});
}

auto current_eval_time() -> std::chrono::milliseconds {
  if (!is_evaluating) return 0s;
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now() - eval_start);
}

auto last_eval_time() -> std::chrono::duration<float32> {
  return _last_eval_time;
}

auto lua_state() -> lua_view {
  return {sol::state_view{lua}, std::unique_lock{mutex}};
}

}  // namespace ensketch::luarepl
