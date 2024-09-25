#pragma once
#include <ensketch/sandbox/defaults.hpp>

namespace ensketch::sandbox {

///
///
template <typename type>
class state_executor {
 public:
  using state_type = type;
  using task_queue = xstd::basic_task_queue<state_type&>;

  /// The constructor creates the inner thread with the main loop function.
  /// The stop token `external` is used to registrate a stop callback.
  /// This allows an external stop source to asynchronously quit the main loop.
  /// All other arguments are forwarded to create a local
  /// state object of `state_type` inside the inner thread.
  /// After its construction the main loop will continuously call the state's
  /// update function and process all enqueued tasks inside the task queue by
  /// providing the local state object as a reference value.
  /// The whole main loop function is wrapped by a try-catch block
  /// that catches any possible exception that might be thrown.
  /// A catched exception will be re-thrown when the object gets destroyed.
  ///
  template <typename... params>
    requires std::constructible_from<state_type, params...>
  state_executor(std::stop_token external, params&&... args)
      : thread{[this, external, ... args = auto(std::forward<params>(args))](
                   std::stop_token internal) {
          // Wrap the main loop around a try-catch block to be able to
          // store and return thrown exceptions to the enclosing thread.
          try {
            // Add a stop callback to an external stop source to also allow
            // external stop signals to quit the thread's main loop.
            // To enable CTAD for `std::stop_callback` template the object
            // is constructed locally and not provided as class member.
            std::stop_callback external_stop{external,
                                             [this] { thread.request_stop(); }};

            // Enable RAII by constructing the state locally inside the thread.
            // This makes sure that `state_type`'s destructor will be called
            // at the end of the scope or if an exception might be thrown.
            state_type state{std::move(args)...};

            // Run the main loop until a stop request has been received.
            while (not internal.stop_requested()) {
              // Process all enqueued tasks in the queue and update the state.
              tasks.process_all(state);
              state.update();
            }
          } catch (...) {
            // Store anything thrown in the promise. This function
            // may throw itself and should then call `std::terminate`.
            promise.set_exception(std::current_exception());
          }
          // Mark the execution as successful.
          promise.set_value();
        }} {}

  /// The destructor first sends a stop request to the inner
  /// thread that runs the main loop to prevent indefinite blocking.
  /// Afterwards, it waits for the future signal to be available and gets it.
  /// In such a way, exceptions that have been thrown in the inner thread
  /// while running the main loop will be re-thrown for further processing.
  ///
  ~state_executor() {
    thread.request_stop();
    // Throw any exception that has been thrown inside the main loop.
    future.get();
  }

  /// Asynchronously send a stop request to the inner thread that
  /// runs the main loop and attempt to quit the state execution.
  ///
  void close() { thread.request_stop(); }

  /// Asynchronously invoke the callable `f` with arguments
  /// `args...` on the task thread in fire-and-forget style.
  /// The function neither blocks nor returns anything.
  ///
  void async_invoke_and_discard(auto&& f, auto&&... args) {
    tasks.async_invoke_and_discard(std::forward<decltype(f)>(f),
                                   std::forward<decltype(args)>(args)...);
  }

  /// Asynchronously invoke `f` with arguments `args...` on the task thread.
  /// The function returns an `std::future` that will contain the return value.
  ///
  [[nodiscard]] auto async_invoke(auto&& f, auto&&... args) {
    return tasks.async_invoke(std::forward<decltype(f)>(f),
                              std::forward<decltype(args)>(args)...);
  }

  /// Asynchronously invoke the callable `f` with arguments `args...` on
  /// the task thread and implicitly convert its return value to `result`.
  /// The function returns an `std::future` that will contain the return value.
  ///
  template <typename result>
  [[nodiscard]] auto async_invoke(auto&& f, auto&&... args) {
    return tasks.template async_invoke<result>(
        std::forward<decltype(f)>(f), std::forward<decltype(args)>(args)...);
  }

  /// Synchronously invoke the callable `f` with
  /// arguments `args...` on the task thread.
  ///
  auto invoke(auto&& f, auto&&... args) {
    auto task = async_invoke(std::forward<decltype(f)>(f),
                             std::forward<decltype(args)>(args)...);
    return task.get();
  }

  /// Synchronously invoke the callable `f` with arguments `args...` on
  /// the task thread and implicitly convert its return value to `result`.
  ///
  template <typename result>
  auto invoke(auto&& f, auto&&... args) {
    auto task = async_invoke<result>(std::forward<decltype(f)>(f),
                                     std::forward<decltype(args)>(args)...);
    return task.get();
  }

 private:
  task_queue tasks{};
  std::promise<void> promise{};
  std::future<void> future{promise.get_future()};
  std::jthread thread{};
};

///
///
template <template <typename> typename policy>
struct executor
    : policy<executor<policy>>,
      state_executor<typename policy<executor<policy>>::state_type> {
  using api = policy<executor<policy>>;
  using state_type = typename api::state_type;
  using base = state_executor<typename api::state_type>;

  ///
  ///
  template <typename... params>
    requires std::constructible_from<state_type, params...>
  explicit executor(params&&... args)
      : base{sandbox::stop_token(), std::forward<params>(args)...} {}
};

}  // namespace ensketch::sandbox
