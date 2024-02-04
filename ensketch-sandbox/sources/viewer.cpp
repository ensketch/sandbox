#include <viewer.hpp>

namespace ensketch::sandbox {

viewer_context::viewer_context(int width, int height) {
  sf::ContextSettings settings;
  settings.majorVersion = 4;
  settings.minorVersion = 5;
  // These values need to be set when 3D rendering is required.
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 4;

  window.create(sf::VideoMode(width, height), "ensketch-sandbox",
                sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);
  window.setActive(true);

  glbinding::initialize(sf::Context::getFunction);
}

viewer::viewer(int width, int height) : viewer_context(width, height) {
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void viewer::process_events() {
  sf::Event event;
  while (window.pollEvent(event)) {
    if (event.type == sf::Event::Closed) _running = false;
  }
}

void viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  window.display();
}

}  // namespace ensketch::sandbox
