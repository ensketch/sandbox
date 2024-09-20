#include <ensketch/luarepl/luarepl.hpp>
#include <ensketch/luarepl/spinners.hpp>
//
#include <ensketch/sandbox/main_thread.hpp>
//
#include <ensketch/sandbox/frame_timer.hpp>
#include <ensketch/sandbox/log.hpp>

namespace ensketch::sandbox {

void add_lua_functions();

namespace luarepl = ensketch::luarepl;

namespace detail {
namespace {
// atomic<bool> done = false;
// thread::id main_thread{};
frame_timer timer{10.0f};
// task_queue chaiscript_tasks{};
// task_queue tasks{};
// ensketch::sandbox::viewer viewer{};
}  // namespace
}  // namespace detail

// static void set_main_thread() noexcept;

// static void process_console_input();

// static void run_main_thread();

inline auto digits_from(std::floating_point auto x,
                        int min_width = 0) -> std::string {
  constexpr czstring digit[] = {"🯰", "🯱", "🯲", "🯳", "🯴",
                                "🯵", "🯶", "🯷", "🯸", "🯹"};
  std::string result{};
  if (x < 0) {
    result += "-";
    x = -x;
  }
  if (x == 0) {
    result += digit[0];
    return result;
  }
  const auto k = static_cast<int>(std::floor(std::log10(x)));
  const auto width = std::max(k, min_width - 1);
  for (size_t i = 0; i <= width; ++i) {
    result = digit[static_cast<size_t>(std::floor(x)) % 10] + result;
    x /= 10;
  }
  return result;
}

static auto prompt() {
  using namespace fmt;
  return fmt::format(
      "\n{}{}{}{}{}{}{}\n{}\n{}  ",  //
      fmt::format(fg(color::gray), "🭅"),
      fmt::format(fg(color::white) | bg(color::gray), "せん {} {}ms",
                  animation(luarepl::dot_spinner, luarepl::current_eval_time()),
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                      luarepl::last_eval_time())
                      .count()),
      fmt::format(fg(color::gray) | bg(color::dim_gray), "🭡"),
      fmt::format(fg(color::white) | bg(color::dim_gray), " {} FPS ",
                  digits_from(detail::timer.fps(), 2)),
      fmt::format(fg(color::dim_gray) | bg(color::dark_gray), "🭡"),
      fmt::format(fg(color::black) | bg(color::dark_gray), " {} ",
                  std::filesystem::current_path().string()),
      fmt::format(fg(color::dark_gray), "🭡"), fmt::format(fg(color::gray), "🭡"),
      fmt::format(fg(color::gray), "🭛"));
}

}  // namespace ensketch::sandbox

int main(int argc, char* argv[]) {
  namespace sandbox = ensketch::sandbox;
  using namespace sandbox;

  luarepl::set_done_and_quit([] { return done(); }, [] { quit(); });
  add_lua_functions();

  for (int i = 1; i < argc; ++i)
    luarepl::async_run_file(std::filesystem::path(argv[i]));

  auto luarepl_task = async_invoke(luarepl::run);

  luarepl::set_prompt(prompt());

  main_thread::run(sandbox::stop_token());
}
