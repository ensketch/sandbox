#pragma once
#include <ensketch/opengl/opengl.hpp>
#include <ensketch/sandbox/defaults.hpp>
//
#include <SFML/Graphics.hpp>

namespace ensketch::sandbox {

class simple_viewer {
 public:
  simple_viewer(int width = 500, int height = 500);
  ~simple_viewer();

  /// Allow the default move constructor and assignment.
  /// Copy constructor and assignment is forbidden.
  /// These lines need to be provided because
  /// the class uses a non-default destructor.
  ///
  simple_viewer(simple_viewer&&) = default;
  simple_viewer& operator=(simple_viewer&&) = default;

  /// Asynchronously attempt to quit the object's main loop.
  ///
  void quit();

  /// Asynchronously set the background color of the window.
  ///
  void set_background_color(float red, float green, float blue);

 private:
  void init();
  void open(int width, int height);
  void close();

  void run();
  void resize(int width, int height);
  void render();

  void receive_events();
  void process(const sf::Event& event);

 private:
  sf::Window window{};
  std::stop_source internal{};
  // std::stop_token external{};
  std::future<void> runner{};
  task_queue tasks{};
};

}  // namespace ensketch::sandbox
