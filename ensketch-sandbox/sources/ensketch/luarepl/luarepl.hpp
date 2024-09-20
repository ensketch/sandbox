#pragma once
#include "defaults.hpp"
//
#include <sol/sol.hpp>

namespace ensketch::luarepl {

///
///
struct lua_view : sol::state_view {
  std::unique_lock<std::mutex> lock{};
};

/// Set the query whether the application is done, denoted by `is_done`,
/// and the function to quit the application, denoted by `shall_quit`.
/// If this function is not called, `luarepl` provides some defaults.
///
void set_done_and_quit(auto&& is_done, auto&& shall_quit);

/// Receive the current prompt as string in a thread-safe manner.
///
auto prompt() -> std::string;

/// Asynchronously set the prompt of the REPL.
///
void set_prompt(std::string_view str);

/// Run `luarepl` main loop. This call blocks.
///
void run();

/// Print given strings to the log of the console.
///
void log(czstring str);
void log(std::string_view str);

///
///
auto lua_state() -> lua_view;

/// Set a given function object as Lua function inside the REPL.
/// This function is available to the user by the given `name`.
///
void set_function(std::string_view name, auto&& f);

/// Evaluate a given Lua file inside the REPL and get its result.
/// Errors are also wrapped by the return value.
/// This function blocks until the execution of the code finishes.
///
auto eval_file(const std::filesystem::path& path)
    -> sol::protected_function_result;

/// Asynchronously run a given Lua file inside the REPL without returning any result.
/// Errors are automatically logged to the REPL via `log`.
///
void async_run_file(const std::filesystem::path& path);

/// Evaluate a given string inside the Lua REPL and get its result.
/// Errors are also wrapped by the return value.
/// This function blocks until the execution of the code finishes.
///
auto eval(std::string_view str)  //
    -> sol::protected_function_result;

/// Asynchronously run a given string inside the Lua REPL.
/// This function does not return any result.
/// Errors are automatically logged to the REPL via `log`.
///
void async_run(std::string_view str);
void async_run(std::string&& str);

///
///
auto current_eval_time() -> std::chrono::milliseconds;

///
///
auto last_eval_time() -> std::chrono::duration<float32>;

}  // namespace ensketch::luarepl

#include "luarepl.ipp"
