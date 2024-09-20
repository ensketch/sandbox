#include <ensketch/sandbox/defaults.hpp>

namespace ensketch::sandbox {

// The application's global stop source is a global and static
// variable that can only be accessed in this source unit.
// All interaction is carried out through the
// functions `quit`, `stop_token`, and `done`.
static std::stop_source stop_source{};

void quit() noexcept {
  stop_source.request_stop();
}

auto stop_token() noexcept -> std::stop_token {
  return stop_source.get_token();
}

bool done() noexcept {
  return stop_source.stop_requested();
}

}  // namespace ensketch::sandbox
