#include <ensketch/sandbox/viewer.hpp>
//
#include <ensketch/sandbox/application.hpp>
//
#include <ensketch/opengl/shader_object.hpp>
#include <ensketch/opengl/shader_program.hpp>
#include <ensketch/opengl/vertex_array.hpp>

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

  glEnable(GL_DEPTH_TEST);

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
  const auto vs = opengl::vertex_shader{R"##(
#version 460 core

vec3 vertices[3] = {
  vec3(-0.5, -0.5, 0.0),
  vec3(0.5, -0.5, 0.0),
  vec3(0.0, 0.5, 0.0)
};

void main() {
  gl_Position = vec4(vertices[gl_VertexID], 1.0);
}
)##"};

  const auto fs = opengl::fragment_shader{R"##(
#version 460 core

layout (location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(1.0);
}
)##"};
  auto shader = opengl::shader_program{vs, fs};

  opengl::vertex_array va{};
  va.bind();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(shader);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  window.display();
}

}  // namespace ensketch::sandbox
