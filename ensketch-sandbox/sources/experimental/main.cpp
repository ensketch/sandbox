#include <ensketch/xstd/xstd.hpp>
// #include "luarepl/luarepl.hpp"

using namespace ensketch::xstd;

template <typename type>
concept signal_parameter = std::copyable<type>;

template <meta::string str, typename... args>
struct signal {
  static consteval auto name() noexcept { return str; }
  static consteval auto arguments() noexcept {
    return meta::type_list<args...>{};
  }
};

template <typename type>
concept generic_signal = requires {
  { type::name() } -> meta::string_instance;
  { type::arguments() } -> meta::type_list_instance;
};

template <typename type, typename signal>
concept slot_for =
    generic_signal<signal> && fold(signal::arguments(), []<typename... args> {
      return std::invocable<type, args...>;
    });

template <generic_signal signal, slot_for<signal>... types>
struct signal_entry : signal, std::tuple<types...> {
  using base = std::tuple<types...>;
  using signal_type = signal;
  constexpr signal_entry(auto&&... args)
      : base{std::forward<decltype(args)>(args)...} {}
  constexpr auto slots() & noexcept { return static_cast<base&>(*this); }
  constexpr auto slots() const& noexcept {
    return static_cast<const base&>(*this);
  }
  constexpr auto slots() && noexcept { return static_cast<base&&>(*this); }
  constexpr auto slots() const&& noexcept {
    return static_cast<const base&&>(*this);
  }
};

template <generic_signal signal>
constexpr auto entry(slot_for<signal> auto&&... slots) {
  return signal_entry<signal, std::unwrap_ref_decay_t<decltype(slots)>...>{
      std::forward<decltype(slots)>(slots)...};
}

constexpr auto table(auto&&... entries) {
  return named_tuple<
      meta::name_list<std::decay_t<decltype(entries)>::name()...>,
      std::tuple<std::unwrap_ref_decay_t<decltype(entries)>...>>{
      std::forward<decltype(entries)>(entries)...};
}

template <meta::string name>
constexpr auto sync_signal(auto&& signals, auto... args) {
  for_each(value<name>(signals).slots(),
           [&](auto slot) { std::invoke(slot, args...); });
}

template <meta::string name>
constexpr auto async_signal(task_queue& queue, auto&& signals, auto... args) {
  for_each(value<name>(signals).slots(), [&](auto slot) {
    queue.push_and_discard([slot, args...] { std::invoke(slot, args...); });
  });
}

template <meta::string str, typename... params>
struct dynamic_signal_entry : std::list<std::function<void(params...)>> {
  using base = std::list<std::function<void(params...)>>;
  static consteval auto name() noexcept { return str; }
  static consteval auto parameters() noexcept {
    return meta::type_list<params...>{};
  }

  constexpr auto slots() & noexcept { return static_cast<base&>(*this); }
  constexpr auto slots() const& noexcept {
    return static_cast<const base&>(*this);
  }
  constexpr auto slots() && noexcept { return static_cast<base&&>(*this); }
  constexpr auto slots() const&& noexcept {
    return static_cast<const base&&>(*this);
  }

  // static constexpr void invoke(std::invocable<params...> auto&& f,
  //                              params... args) {
  //   std::invoke(std::forward<decltype(f)>(f), args...);
  // }

  // void operator()(params... args) const {
  //   std::ranges::for_each(*this, [&](auto& slot) {});
  // }
};

template <meta::string name, typename... params>
constexpr auto emit(dynamic_signal_entry<name, params...>& signal,
                    task_queue& queue,
                    params... args) {
  std::ranges::for_each(signal.slots(), [&](auto& slot) {
    queue.push_and_discard([&] { std::invoke(slot, args...); });
  });
}

template <meta::string name>
constexpr auto emit_signal(auto& signals, task_queue& queue, auto... args) {
  emit(value<name>(signals), queue, args...);
}

auto string_from(std::source_location location) -> std::string {
  return std::format(
      "{}:{}:{}:{}",
      relative(std::filesystem::path(location.file_name())).string(),
      location.line(), location.column(), location.function_name());
}

struct debug_mutex_timeout : std::runtime_error {
  using base = std::runtime_error;
  debug_mutex_timeout(std::source_location owner, std::source_location attempt)
      : base{std::format(
            "timeout during attempt to lock mutex at {} previously owned at {}",
            string_from(attempt),
            string_from(owner))} {}
};

template <auto timeout_ms>
struct debug_mutex {
  void lock(std::source_location location = std::source_location::current()) {
    if (!mutex.try_lock_for(std::chrono::milliseconds{timeout_ms}))
      throw debug_mutex_timeout{source, location};
    source = location;
  }

  bool try_lock() { return mutex.try_lock(); }

  template <typename rep, typename period>
  bool try_lock_for(
      const std::chrono::duration<rep, period>& timeout_duration) {
    return mutex.try_lock_for(timeout_duration);
  }

  template <typename clock, typename duration>
  bool try_lock_until(
      const std::chrono::time_point<clock, duration>& timeout_time) {
    return mutex.try_lock_until(timeout_time);
  }

  void unlock() { mutex.unlock(); }

  std::timed_mutex mutex{};
  std::source_location source{};
};

template <typename Mutex>
struct debug_lock {
  using mutex_type = Mutex;
  mutex_type& mutex;
  explicit debug_lock(
      mutex_type& m,
      std::source_location location = std::source_location::current())
      : mutex{m} {
    mutex.lock(location);
  }
  debug_lock(const debug_lock&) = delete;
  ~debug_lock() { mutex.unlock(); }
};

// std::mutex mutex1;
// std::mutex mutex2;
using mutex_type = debug_mutex<2000>;
mutex_type mutex1;
mutex_type mutex2;
int main() {
  // auto p1 = async([] {
  //   debug_lock lock1{mutex1};
  //   std::this_thread::sleep_for(1s);
  //   debug_lock lock2{mutex2};
  // });
  // auto p2 = async([] {
  //   debug_lock lock2{mutex2};
  //   std::this_thread::sleep_for(1s);
  //   debug_lock lock1{mutex1};
  // });
  // p1.get();
  // p2.get();

  // task_queue tasks{};

  // dynamic_signal_entry<"number", int> number_signal{};

  // auto signals = table(dynamic_signal_entry<"quit">{},
  //                      dynamic_signal_entry<"number", int>{});

  // value<"quit">(signals).push_back([] { std::println("First quit slot."); });
  // value<"quit">(signals).push_back([] { std::println("Second quit slot."); });

  // emit_signal<"quit">(signals, tasks);

  // value<"quit">(signals).pop_back();

  // emit_signal<"quit">(signals, tasks);

  // number_signal.emplace_back(
  //     [](int x) { std::println("First number slot. x = {}", x); });
  // number_signal.emplace_back(
  //     [](int x) { std::println("Second number slot. x = {}", x); });

  // emit(number_signal, tasks, 10);
  // emit_signal<"number">(signals, tasks, 13);

  // auto signals =
  //     table(entry<signal<"quit">>([] { std::println("Quit the application."); },
  //                                 [] { std::println("Second quit slot"); }),
  //           entry<signal<"number", int>>(
  //               [](int x) { std::println("first number slot x = {}.", x); },
  //               [](int x) { std::println("Second number slot x = {}.", x); }));

  // async_signal<"quit">(tasks, signals);
  // async_signal<"number">(tasks, signals, 10);
  // async_signal<"number">(tasks, signals, 13);

  // auto p1 = async([&] { tasks.process_all(); });
  // auto p2 = async([&] { tasks.process_all(); });
  // tasks.process_all();

  // using namespace ensketch::xstd;
  // namespace luarepl = ensketch::luarepl;
  // std::atomic<bool> done{false};
  // luarepl::set_done_and_quit([&done] { return done.load(); },
  //                            [&done] { done = true; });
  // luarepl::set_function("quit", [] { luarepl::quit(); });
  // luarepl::set_function("print", [](czstring str) { luarepl::log(str); });
  // auto repl_task = async(luarepl::run);
}
