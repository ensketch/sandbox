#pragma once
#include <ensketch/opengl/opengl.hpp>
//
#include <SFML/Graphics.hpp>
//
#include <ensketch/sandbox/polyhedral_surface.hpp>

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

  //
  bool view_should_update = false;

  // World Origin
  vec3 origin;
  // Basis Vectors of Right-Handed Coordinate System
  vec3 up{0, 1, 0};
  vec3 right{1, 0, 0};
  vec3 front{0, 0, 1};
  // Spherical/Horizontal Coordinates of Camera
  float radius = 10;
  float altitude = 0;
  float azimuth = 0;

  opengl::camera camera{};

  struct device_storage {
    opengl::shader_program shader{};
    opengl::vertex_array va{};
  };
  optional<device_storage> device{};

  // polyhedral_surface surface{};
  // scene surface{};
};

}  // namespace ensketch::sandbox
