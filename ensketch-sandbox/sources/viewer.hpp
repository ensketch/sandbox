#pragma once
#include <SFML/Graphics.hpp>
//
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
//
#include <glm/glm.hpp>
//
#include <glm/ext.hpp>
//
#include <glm/gtx/norm.hpp>

namespace ensketch::sandbox {

using namespace gl;

class viewer_context {
 public:
  viewer_context(int width, int height);

  void set_vsync(bool value = true) noexcept {
    window.setVerticalSyncEnabled(value);
  }

 protected:
  sf::Window window{};
};

class viewer : public viewer_context {
 public:
  viewer(int width, int height);

  void resize();
  void resize(int width, int height);
  void process_events();
  void update();
  void update_view();
  void render();
  void run();

  bool running() const noexcept { return _running; }

 private:
  bool _running = true;
};

}  // namespace ensketch::sandbox
