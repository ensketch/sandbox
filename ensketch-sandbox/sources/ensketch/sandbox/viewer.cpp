#include <ensketch/sandbox/viewer.hpp>
//
#include <ensketch/sandbox/application.hpp>

namespace ensketch::sandbox {

namespace {
void /*APIENTRY*/ glDebugOutput(GLenum source,
                                GLenum type,
                                unsigned int id,
                                GLenum severity,
                                GLsizei length,
                                const char* message,
                                const void* userParam) {
  ensketch::sandbox::app().info(message);
}
}  // namespace

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

  _running = true;
  init();
}

void viewer::close() {
  free();
  window.close();
  _running = false;
}

void viewer::init() {
  glbinding::initialize(sf::Context::getFunction);

  int flags;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & static_cast<int>(GL_CONTEXT_FLAG_DEBUG_BIT)) {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                          GL_TRUE);
  }

  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void viewer::free() {}

void viewer::process_events() {
  sf::Event event;
  while (window.pollEvent(event)) {
    if (event.type == sf::Event::Closed) {
      app().quit();
    } else if (event.type == sf::Event::KeyPressed) {
      switch (event.key.code) {
        case sf::Keyboard::Escape:
          app().quit();
          break;
      }
    }
  }
}

void viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  window.display();
}

}  // namespace ensketch::sandbox
