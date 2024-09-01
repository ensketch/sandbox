#pragma once
#include <ensketch/sandbox/utility.hpp>
//
#include <ensketch/sandbox/task_queue.hpp>
#include <ensketch/sandbox/viewer.hpp>

namespace ensketch::sandbox {

// Return the main thread id of the application.
//
auto main_thread() noexcept -> thread::id;

// Attempt to quit the application in a thread-safe manner.
//
void quit() noexcept;

//
//
bool not_done() noexcept;

//
//
auto main_task_queue() -> task_queue&;

//
//
// auto path_from_lookup(const filesystem::path& path) -> filesystem::path {
//   if (path.is_absolute()) return path;
//   if (lookup_path.empty()) return path;
//   return lookup_path / path;
// }

//
//
void eval_chaiscript(const string& code);
void eval_chaiscript(const filesystem::path& script);
bool process_chaiscript_tasks();

void add_chaiscript_functions();

void eval_lua(const string& code);
void eval_lua(const filesystem::path& script);
bool process_lua_tasks();
void add_lua_functions();

auto main_viewer() noexcept -> viewer&;

void open_viewer(int width, int height);
void close_viewer();

}  // namespace ensketch::sandbox
