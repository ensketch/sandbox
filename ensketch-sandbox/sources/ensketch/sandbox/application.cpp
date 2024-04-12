#include <ensketch/sandbox/application.hpp>

namespace ensketch::sandbox {

auto app() noexcept -> application& {
  static application _app{};
  return _app;
}

application::application() {
  debug("Successfully constructed application.");
}

application::~application() noexcept {
  debug("Successfully destroyed application\n");
}

void application::run() {
  {
    scoped_lock lock{run_mutex};
    if (run_thread != thread::id{})
      throw runtime_error("Failed to run application! It is already running.");
    run_thread = this_thread::get_id();
  }

  interpreter_task = async(launch::async, [this] {
    init_interpreter_module();
    interpreter.run();
  });

  running = true;

  while (running) {
    console.capture(format("FPS = {:6.2f}\n", timer.fps()));

    process_console_input();
    process_eval_chaiscript_task();
    process_task_queue();

    if (viewer) {
      viewer.process_events();
      viewer.update();
      viewer.render();
    }

    console.update();
    timer.update();
  }

  console.abort_input();
  interpreter.quit();

  running = false;
  run_thread = {};
}

void application::quit() {
  running = false;
}

void application::process_console_input() {
  if (const auto ready = console.async_input()) {
    const auto tmp = ready.value();
    if (!tmp) {
      running = false;
      return;
    }
    string input = tmp;
    if (input.empty()) return;
    console.log("\n");
    // eval_chaiscript(input);
    auto task = interpreter.future_from_task_queue(
        [this, input] { interpreter.eval_chaiscript(input); });
  }
}

void application::init_interpreter_module() {
  using namespace chaiscript;

  // Construct a static vector of all ChaiScript functions.
  // It needs to be static as the help function will need to refer to it.
  //
  static vector<
      tuple<string, string, sandbox::interpreter::chaiscript_value_type>>
      objects{
          {"help", "Print available functions.", var(fun([&] {
             stringstream out{};
             out << "Available Functions:\n";
             for (const auto& [name, help, data] : objects)
               out << '\t' << name << '\n' << "\t\t" << help << "\n\n";
             app().info(out.str());
           }))},

          {"quit", "Quit the application.", var(fun([this] { quit(); }))},

          {"open_viewer", "Open the viewer with an OpenGL context.",
           var(fun(
               [this](int width, int height) { open_viewer(width, height); }))},

          {"close_viewer", "Close the viewer and OpenGL context.",
           var(fun([this]() { close_viewer(); }))},

          {"store_image_from_viewer",
           "Stores the viewer's current framebuffer as image.",
           var(fun([this](const string& path) {
             // viewer.store_image(path);
             auto task = future_from_task_queue(
                 [this, path] { viewer.store_image_from_view(path); });
             task.wait();
           }))},

          {"load_surface", "Load a surface mesh from file.",
           var(fun([this](const string& path) {
             // viewer.load_surface(path);
             auto task = future_from_task_queue(
                 [this, path] { viewer.load_surface(path); });
             task.wait();
           }))},

          {"save_perspective", "Save camera perspective to given file.",
           var(fun(
               [this](const string& path) { viewer.save_perspective(path); }))},

          {"load_perspective", "Load camera perspective from given file.",
           var(fun([this](const string& path) {
             auto task = future_from_task_queue(
                 [this, path] { viewer.load_perspective(path); });
             task.wait();
           }))},

          {"save_surface_vertex_curve",
           "Save current surface vertex curve to given file.",
           var(fun([this](const string& path) {
             viewer.save_surface_vertex_curve(path);
           }))},

          {"load_surface_vertex_curve",
           "Load surface vertex curve from given file.",
           var(fun([this](const string& path) {
             auto task = future_from_task_queue(
                 [this, path] { viewer.load_surface_vertex_curve(path); });
             task.wait();
           }))},
      };

  // Add all module functions to the current ChaiScript thread.
  //
  for (const auto& [name, help, data] : objects) interpreter.add(data, name);
}

void application::async_eval_chaiscript(const filesystem::path& path) {
  if (eval_chaiscript_task.valid()) {
    error(format(
        "Failed to start asynchronous evaluation of ChaiScript file.\n Another "
        "file is currently evaluated.\nfile='{}'",
        path.string()));
    return;
  }

  eval_chaiscript_task = interpreter.future_from_task_queue([this, path] {
    const auto abs_path = absolute(path);
    lookup_path = abs_path.parent_path();
    interpreter.eval_chaiscript(abs_path);
    lookup_path = filesystem::path{};
  });

  info(
      format("Started asynchronous evaluation of ChaiScript file.\nfile = '{}'",
             path.string()));
}

void application::process_eval_chaiscript_task() {
  if (!eval_chaiscript_task.valid()) return;
  if (future_status::ready != eval_chaiscript_task.wait_for(0s)) return;
  eval_chaiscript_task = {};
  info("Sucessfully finished asynchronous evaluation of ChaiScript file.");
}

void application::process_task_queue() {
  packaged_task<void()> task{};
  {
    scoped_lock lock{task_queue_mutex};
    if (task_queue.empty()) return;
    task = move(task_queue.front());
    task_queue.pop();
  }
  task();
}

void application::basic_open_viewer(int width, int height) {
  viewer.open(width, height);
  viewer.set_vsync();
  timer.set_syncing(false);
}

auto application::async_open_viewer(int width, int height) -> future<void> {
  return future_from_task_queue(
      [this, width, height] { basic_open_viewer(width, height); });
}

void application::open_viewer(int width, int height) {
  if (this_thread::get_id() == running_thread()) {
    basic_open_viewer(width, height);
    return;
  }
  auto task = async_open_viewer(width, height);
  task.wait();
}

void application::basic_close_viewer() {
  viewer.close();
  timer.set_syncing();
}

auto application::async_close_viewer() -> future<void> {
  return future_from_task_queue([this] { basic_close_viewer(); });
}

void application::close_viewer() {
  if (this_thread::get_id() == running_thread()) {
    basic_close_viewer();
    return;
  }
  auto task = async_close_viewer();
  task.wait();
}

}  // namespace ensketch::sandbox
