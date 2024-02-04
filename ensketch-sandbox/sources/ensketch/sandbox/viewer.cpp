#include <ensketch/sandbox/viewer.hpp>

namespace ensketch::sandbox {

sf::ContextSettings viewer::opengl_context_settings{
    /*.depthBits = */ 24,
    /*.stencilBits = */ 8,
    /*.antialiasingLevel = */ 4,
    /*.majorVersion = */ 4,
    /*.minorVersion = */ 6,
    /*.attributeFlags = */
    sf::ContextSettings::Core | sf::ContextSettings::Debug,
    /*.sRgbCapable = */ false};

void viewer::open(int width, int height) {
  window.create(sf::VideoMode(width, height),  //
                "ensketch-sandbox",            //
                sf::Style::Default,            //
                opengl_context_settings);

  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);
  window.setActive(true);

  glbinding::initialize(sf::Context::getFunction);

  _running = true;
  init();
}

void viewer::close() {
  free();
  window.close();
  _running = false;
}

void viewer::init() {
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void viewer::free() {}

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
