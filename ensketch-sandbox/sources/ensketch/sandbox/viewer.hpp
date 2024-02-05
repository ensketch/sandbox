#pragma once
#include <ensketch/opengl/utility.hpp>
//
#include <SFML/Graphics.hpp>

namespace ensketch::sandbox {

using namespace gl;

class viewer {
 public:
  static sf::ContextSettings opengl_context_settings;

  viewer() noexcept {}

  void open(int width, int height);
  void close();

  bool is_open() const noexcept { return window.isOpen(); }
  operator bool() const noexcept { return is_open(); }

  void init();
  void free();

  void resize();
  void resize(int width, int height);
  void process_events();
  void update();
  void update_view();
  void render();
  void run();

  bool running() const noexcept { return _running; }

  void set_vsync(bool value = true) noexcept {
    window.setVerticalSyncEnabled(value);
  }

 private:
  bool _running = false;
  sf::Window window{};
};

}  // namespace ensketch::sandbox
