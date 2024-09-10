#include <ensketch/luarepl/luarepl.hpp>
//
#include <ensketch/sandbox/main.hpp>
//
#include <ensketch/sandbox/console.hpp>
#include <ensketch/sandbox/frame_timer.hpp>
#include <ensketch/sandbox/log.hpp>

namespace ensketch::sandbox {

namespace detail {
namespace {
atomic<bool> done = false;
thread::id main_thread{};
frame_timer timer{10.0f};
// task_queue chaiscript_tasks{};
task_queue tasks{};
ensketch::sandbox::viewer viewer{};
}  // namespace
}  // namespace detail

static void set_main_thread() noexcept;

static void process_console_input();

static void run_main_thread();

}  // namespace ensketch::sandbox

int main(int argc, char* argv[]) {
  namespace luarepl = ensketch::luarepl;
  namespace sandbox = ensketch::sandbox;
  using namespace ensketch::xstd;
  // using namespace ensketch::sandbox;

  sandbox::set_main_thread();

  // for (int i = 1; i < argc; ++i) eval_chaiscript(filesystem::path(argv[i]));
  // for (int i = 1; i < argc; ++i) eval_lua(filesystem::path(argv[i]));

  luarepl::set_done_and_quit([] { return !sandbox::not_done(); },
                             [] { sandbox::quit(); });
  // luarepl::set_function("quit", [] { sandbox::quit(); });
  // luarepl::set_function("print", [](czstring str) { luarepl::log(str); });

  luarepl::set_function("open_viewer", [](int width, int height) {
    sandbox::open_viewer(width, height);
  });
  luarepl::set_function("close_viewer", [] { sandbox::close_viewer(); });
  luarepl::set_function("load_surface", [](const std::string& path) {
    auto task = sandbox::main_task_queue().push(
        [path] { sandbox::main_viewer().load_surface(path); });
    task.wait();
  });

  auto luarepl_task = async(luarepl::run);

  // while (not done) {
  //   std::this_thread::sleep_for(1s);
  //   luarepl::log("tick");
  // }

  // jthread console_thread{[] {
  //   console::init();
  //   while (not_done()) {
  //     process_console_input();
  //   }
  // }};

  // jthread lua_thread{[] {
  //   while (not_done()) {
  //     while (not_done() && lua_state.process());
  //     this_thread::sleep_for(100ms);
  //   }
  // }};

  sandbox::run_main_thread();
}

namespace ensketch::sandbox {

static void set_main_thread() noexcept {
  detail::main_thread = this_thread::get_id();
}

auto main_thread() noexcept -> thread::id {
  return detail::main_thread;
}

void quit() noexcept {
  detail::done = true;
}

bool not_done() noexcept {
  return !detail::done.load();
}

auto main_task_queue() -> task_queue& {
  return detail::tasks;
}

auto main_viewer() noexcept -> viewer& {
  return detail::viewer;
}

void open_viewer(int width, int height) {
  auto task = detail::tasks.push([width, height] {
    detail::viewer.open(width, height);
    detail::viewer.set_vsync();
    detail::timer.set_syncing(false);
  });
  task.wait();
}

void close_viewer() {
  auto task = detail::tasks.push([] {
    detail::viewer.close();
    detail::timer.set_syncing();
  });
  task.wait();
}

// static void process_console_input() {
//   const auto str = console::input();
//   if (!str) {
//     quit();
//     return;
//   }
//   string input = str;
//   if (input.empty()) return;
//   console::log("\n");
//   lua_state.eval(input);
// }

static void run_main_thread() {
  while (not_done()) {
    detail::tasks.process();
    // console::capture(format("FPS = {:6.2f}\n", detail::timer.fps()));
    // console::update();
    detail::timer.update();
    if (detail::viewer) {
      detail::viewer.process_events();
      detail::viewer.update();
      detail::viewer.render();
    }
  }
}

}  // namespace ensketch::sandbox
