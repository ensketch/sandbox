#include "luarepl/luarepl.hpp"

int main() {
  using namespace ensketch::xstd;
  namespace luarepl = ensketch::luarepl;

  std::atomic<bool> done{false};
  luarepl::set_done_and_quit([&done] { return done.load(); },
                             [&done] { done = true; });

  luarepl::set_function("quit", [] { luarepl::quit(); });
  luarepl::set_function("print", [](czstring str) { luarepl::log(str); });

  auto repl_task = async(luarepl::run);

  while (not done) {
    std::this_thread::sleep_for(1s);
    luarepl::log("tick");
  }
}
