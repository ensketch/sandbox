#include <ensketch/sandbox/interpreter.hpp>
//
#include <ensketch/sandbox/application.hpp>
//
#include <chaiscript/chaiscript.hpp>

using namespace chaiscript;

namespace ensketch::sandbox {

namespace {

ChaiScript chai{};

}  // namespace

void interpreter::run() {
  {
    scoped_lock lock{run_mutex};
    if (run_thread != thread::id{})
      throw runtime_error("Failed to run interpreter! It is already running.");
    run_thread = this_thread::get_id();
  }

  running = true;

  while (running) {
    process_task_queue();
  }

  running = false;
  run_thread = {};
}

void interpreter::quit() {
  running = false;
}

void interpreter::process_task_queue() {
  packaged_task<void()> task{};
  {
    scoped_lock lock{task_queue_mutex};
    if (task_queue.empty()) return;
    task = move(task_queue.front());
    task_queue.pop();
  }
  task();
}

void interpreter::eval_chaiscript(const filesystem::path& script) const try {
  chai.eval_file(script);
} catch (chaiscript::exception::eval_error& e) {
  app().error(format("ChaiScript File Eval {}", e.what()));
}

void interpreter::eval_chaiscript(const string& code) const try {
  chai.eval(code);
} catch (chaiscript::exception::eval_error& e) {
  app().error(format("ChaiScript String Eval {}", e.what()));
}

void interpreter::add(chaiscript_value_type value, const string& name) {
  chai.add(value, name);
}

void interpreter::add(chaiscript_function_type function, const string& name) {
  chai.add(function, name);
}

}  // namespace ensketch::sandbox
