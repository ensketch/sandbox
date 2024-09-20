#pragma once
#include <ensketch/sandbox/defaults.hpp>

namespace ensketch::sandbox::main_thread {

// Return the main thread id of the application.
//
auto id() noexcept -> std::thread::id;

///
///
void run(std::stop_token stop_token);

///
///
extern task_queue tasks;

/// Asynchronously invoke the callable `f` with arguments
/// `args...` on the main thread in fire-and-forget style.
/// The function neither blocks nor returns anything.
///
inline void async_invoke_and_discard(auto&& f, auto&&... args) {
  tasks.async_invoke_and_discard(std::forward<decltype(f)>(f),
                                 std::forward<decltype(args)>(args)...);
}

/// Asynchronously invoke `f` with arguments `args...` on the main thread.
/// The function returns an `std::future` that will contain the return value.
///
[[nodiscard]] inline auto async_invoke(auto&& f, auto&&... args) {
  return tasks.async_invoke(std::forward<decltype(f)>(f),
                            std::forward<decltype(args)>(args)...);
}

/// Asynchronously invoke the callable `f` with arguments `args...` on
/// the main thread and implicitly convert its return value to `result`.
/// The function returns an `std::future` that will contain the return value.
///
template <typename result>
[[nodiscard]] inline auto async_invoke(auto&& f, auto&&... args) {
  return tasks.template async_invoke<result>(
      std::forward<decltype(f)>(f), std::forward<decltype(args)>(args)...);
}

/// Synchronously invoke the callable `f` with
/// arguments `args...` on the main thread.
/// If the function is already called on the main thread, it simply
/// forwards to `std::invoke` to prevent indefinite blocking.
///
inline auto invoke(auto&& f, auto&&... args) {
  // Forward to `std::invoke` when called on main thread.
  if (main_thread::id() == std::this_thread::get_id())
    return std::invoke(std::forward<decltype(f)>(f),
                       std::forward<decltype(args)>(args)...);
  // Otherwise, enqueue callable as task for asynchronous invocation.
  // Wait for the retrieved `std::future` to be available.
  auto task = main_thread::async_invoke(std::forward<decltype(f)>(f),
                                        std::forward<decltype(args)>(args)...);
  return task.get();
}

/// Synchronously invoke the callable `f` with arguments `args...` on
/// the main thread and implicitly convert its return value to `result`.
/// If the function is already called on the main thread, it simply
/// forwards to `std::invoke_r` to prevent indefinite blocking.
///
template <typename result>
inline auto invoke(auto&& f, auto&&... args) {
  // Forward to `std::invoke_r` when called on main thread.
  if (main_thread::id() == std::this_thread::get_id())
    return std::invoke_r<result>(std::forward<decltype(f)>(f),
                                 std::forward<decltype(args)>(args)...);
  // Otherwise, enqueue callable as task for asynchronous invocation.
  // Wait for the retrieved `std::future` to be available.
  auto task = main_thread::async_invoke<result>(
      std::forward<decltype(f)>(f), std::forward<decltype(args)>(args)...);
  return task.get();
}

}  // namespace ensketch::sandbox::main_thread
