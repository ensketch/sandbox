#pragma once
#include <ensketch/sandbox/utility.hpp>

namespace ensketch::sandbox {

class frame_timer {
 public:
  using real = float;
  using clock = chrono::high_resolution_clock;
  using time_point = chrono::time_point<clock>;
  using duration = chrono::duration<real>;

  frame_timer() noexcept {}
  // frame_timer(const duration& d) noexcept {}
  frame_timer(real fps) noexcept
      : desired_frame_time{1 / fps}, _syncing{true} {}

  auto frame_time() const noexcept { return current_frame_time; }

  auto fps() const noexcept { return 1 / frame_time().count(); }

  bool syncing() const noexcept { return _syncing; }

  void set_syncing(bool value = true) noexcept { _syncing = value; }

  void sync() noexcept {
    const auto work_time = clock::now() - _time;
    const auto idle_time = desired_frame_time - work_time;
    this_thread::sleep_for(idle_time);
  }

  void update() noexcept {
    if (syncing()) sync();
    const auto new_time = clock::now();
    current_frame_time = new_time - _time;
    _time = new_time;
  }

 private:
  time_point _time = clock::now();
  duration current_frame_time{};
  duration desired_frame_time{};
  bool _syncing = false;
};

}  // namespace ensketch::sandbox
