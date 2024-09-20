#include <ensketch/sandbox/simple_viewer.hpp>
//
#include <ensketch/sandbox/main_thread.hpp>

namespace ensketch::sandbox {

simple_viewer::simple_viewer(int width, int height) {
  // The `std::future<void>` object `runner` is used to handle the parallel
  // execution of the main loop and to make the constructor non-blocking.
  runner = async_invoke([this, width, height] {
    open(width, height);
    run();
    window.close();
  });
}

simple_viewer::~simple_viewer() {
  // The destructor should not be blocking execution.
  // Thus, call `quit` make it return from the main loop.
  quit();
  // The `std::future<void> runner` does not contain an actual value
  // but may throw an exception if main loop's execution failed.
  runner.get();
}

void simple_viewer::quit() {
  internal.request_stop();
}

void simple_viewer::set_background_color(float red, float green, float blue) {
  tasks.push_and_discard(
      [this, red, green, blue] { glClearColor(red, green, blue, 1.0f); });
}

void simple_viewer::open(int width, int height) {
  // On MacOS, windows and events must be managed in the main thread.
  // See: https://www.sfml-dev.org/tutorials/2.6/window-window.php
  main_thread::invoke([this, width, height] {
    sf::ContextSettings opengl_context_settings{
        /*.depthBits = */ 24,
        /*.stencilBits = */ 8,
        /*.antialiasingLevel = */ 4,
        /*.majorVersion = */ 4,
        /*.minorVersion = */ 6,
        /*.attributeFlags = */
        sf::ContextSettings::Core | sf::ContextSettings::Debug,
        /*.sRgbCapable = */ false};

    window.create(sf::VideoMode(width, height),        //
                  "ensketch::sandbox::simple_viewer",  //
                  sf::Style::Default,                  //
                  opengl_context_settings);
    // "A window is active only on the current thread,
    // if you want to make it active on another thread
    // you have to deactivate it on the previous thread
    // first if it was active."
    // See: https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1Window.php
    window.setActive(false);
  });

  // Activate the window as the current target for OpenGL rendering.
  // Make the window's OpenGL context available on the current thread.
  window.setActive(true);

  // Initialize the window's default settings.
  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);

  // Initialize all OpenGL functions with `glbinding` on the current thread.
  glbinding::initialize(sf::Context::getFunction);

  // Initialize the default OpenGL state.
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(10.0f);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void simple_viewer::init() {}

void simple_viewer::close() {
  window.close();
}

void simple_viewer::run() {
  while (not(internal.stop_requested() || sandbox::done())) {
    receive_events();
    tasks.process_all();
    render();
    window.display();
  }
}

void simple_viewer::resize(int width, int height) {
  glViewport(0, 0, width, height);
}

void simple_viewer::receive_events() {
  // Events must be polled in the window's thread and, especially
  // on MacOS, windows and events must be managed in the main thread.
  // See: https://www.sfml-dev.org/tutorials/2.6/window-window.php
  main_thread::invoke([this] {
    sf::Event event;
    while (window.pollEvent(event))
      // The actual handling of a specific event will be enqueued on the
      // viewer's task queue to be handled on the viewer's thread directly.
      tasks.async_invoke_and_discard(&simple_viewer::process, this, event);
  });
}

void simple_viewer::process(const sf::Event& event) {
  if (event.type == sf::Event::Closed)
    sandbox::quit();
  else if (event.type == sf::Event::Resized)
    resize(event.size.width, event.size.height);
}

void simple_viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}  // namespace ensketch::sandbox
