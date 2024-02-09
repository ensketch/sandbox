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
  if (is_open()) close();

  window.create(sf::VideoMode(width, height),  //
                "ensketch-sandbox",            //
                sf::Style::Default,            //
                opengl_context_settings);

  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);
  window.setActive(true);

  _running = true;
  init();

  // const auto s = window.getSize();
  resize(width, height);
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

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  device = device_storage{};

  const auto vs = opengl::vertex_shader{R"##(
#version 460 core

uniform mat4 projection;
uniform mat4 view;

vec3 vertices[3] = {
  vec3(-0.5, -0.5, 0.0),
  vec3(0.5, -0.5, 0.0),
  vec3(0.0, 0.5, 0.0)
};

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;

out vec3 normal;

void main() {
  // gl_Position = projection * view * vec4(vertices[gl_VertexID], 1.0);
  gl_Position = projection * view * vec4(p, 1.0);
  normal = vec3(view * vec4(n, 0.0));
}
)##"};

  const auto fs = opengl::fragment_shader{R"##(
#version 460 core

in vec3 normal;

layout (location = 0) out vec4 frag_color;

void main() {
  // frag_color = vec4(1.0);
  vec3 n = normalize(normal);
  vec3 view_dir = vec3(0.0, 0.0, 1.0);
  vec3 light_dir = view_dir;
  float d = max(dot(light_dir, n), 0.0);
  vec3 color = vec3(vec2(0.2 + 1.0 * pow(d,1000) + 0.75 * pow(d,0.2)),1.0);
  frag_color = vec4(color, 1.0);
}
)##"};

  device->shader = opengl::shader_program{vs, fs};

  device->va.bind();
  device->vertices.bind();
  device->faces.bind();

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(polyhedral_surface::vertex),
                        (void*)offsetof(polyhedral_surface::vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                        sizeof(polyhedral_surface::vertex),
                        (void*)offsetof(polyhedral_surface::vertex, normal));

  surface_should_update = true;
}

void viewer::free() {
  device.reset();
}

void viewer::resize(int width, int height) {
  glViewport(0, 0, width, height);
  camera.set_screen_resolution(width, height);
  view_should_update = true;
}

void viewer::process_events() {
  // Get new mouse position and compute movement in space.
  const auto new_mouse_pos = sf::Mouse::getPosition(window);
  const auto mouse_move = new_mouse_pos - mouse_pos;
  mouse_pos = new_mouse_pos;

  sf::Event event;
  while (window.pollEvent(event)) {
    if (event.type == sf::Event::Closed)
      app().quit();
    else if (event.type == sf::Event::Resized)
      resize(event.size.width, event.size.height);
    else if (event.type == sf::Event::MouseWheelScrolled)
      zoom(0.1 * event.mouseWheelScroll.delta);
    else if (event.type == sf::Event::MouseButtonPressed) {
      switch (event.mouseButton.button) {
        case sf::Mouse::Middle:
          // look_at(event.mouseButton.x, event.mouseButton.y);
          break;
      }
    } else if (event.type == sf::Event::KeyPressed) {
      switch (event.key.code) {
        case sf::Keyboard::Escape:
          app().quit();
          break;
        case sf::Keyboard::Num1:
          set_y_as_up();
          break;
        case sf::Keyboard::Num2:
          set_z_as_up();
          break;
      }
    }
  }

  if (window.hasFocus()) {
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
        shift({mouse_move.x, mouse_move.y});
      else
        turn({-0.01 * mouse_move.x, 0.01 * mouse_move.y});
    }
  }
}

void viewer::update() {
  handle_surface_load_task();

  if (view_should_update) {
    update_view();
    view_should_update = false;
  }

  if (surface_should_update) {
    // surface.update();

    device->vertices.allocate_and_initialize(surface.vertices);
    device->faces.allocate_and_initialize(surface.faces);

    surface_should_update = false;
  }
}

void viewer::update_view() {
  // Compute camera position by using spherical coordinates.
  // This transformation is a variation of the standard
  // called horizontal coordinates often used in astronomy.
  //
  auto p = cos(altitude) * sin(azimuth) * right +  //
           cos(altitude) * cos(azimuth) * front +  //
           sin(altitude) * up;
  p *= radius;
  p += origin;
  camera.move(p).look_at(origin, up);

  // camera.set_near_and_far(std::max(1e-3f * radius, radius - bounding_radius),
  //                         radius + bounding_radius);
  camera.set_near_and_far(0.1f, 100.0f);

  // shaders.apply([this](opengl::shader_program_handle shader) {
  //   shader.bind()
  //       .try_set("projection", camera.projection_matrix())
  //       .try_set("view", camera.view_matrix())
  //       .try_set("viewport", camera.viewport_matrix())
  //       .try_set("viewport_width", camera.screen_width())
  //       .try_set("viewport_height", camera.screen_height());
  // });

  if (device)
    device->shader.bind()
        .try_set("projection", camera.projection_matrix())
        .try_set("view", camera.view_matrix());
}

void viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  device->va.bind();
  device->faces.bind();
  device->shader.bind();
  glDrawElements(GL_TRIANGLES, 3 * surface.faces.size(), GL_UNSIGNED_INT, 0);
  // glDrawArrays(GL_TRIANGLES, 0, 3);

  window.display();
}

void viewer::turn(const vec2& angle) {
  altitude += angle.y;
  azimuth += angle.x;
  constexpr float bound = pi / 2 - 1e-5f;
  altitude = std::clamp(altitude, -bound, bound);
  view_should_update = true;
}

void viewer::shift(const vec2& pixels) {
  const auto shift = -pixels.x * camera.right() + pixels.y * camera.up();
  const auto scale = camera.pixel_size() * radius;
  origin += scale * shift;
  view_should_update = true;
}

void viewer::zoom(float scale) {
  radius *= exp(-scale);
  view_should_update = true;
}

// void viewer::look_at(float x, float y) {
//   const auto r = camera.primary_ray(x, y);
//   if (const auto p = intersection(r, surface)) {
//     origin = r(p.t);
//     radius = p.t;
//     view_should_update = true;
//   }
// }

void viewer::set_z_as_up() {
  right = {1, 0, 0};
  front = {0, -1, 0};
  up = {0, 0, 1};
  view_should_update = true;
}

void viewer::set_y_as_up() {
  right = {1, 0, 0};
  front = {0, 0, 1};
  up = {0, 1, 0};
  view_should_update = true;
}

void viewer::load_surface(const filesystem::path& path) {
  try {
    const auto load_start = clock::now();

    surface = polyhedral_surface_from(path);
    // surface.generate_edges();

    const auto load_end = clock::now();

    // Evaluate loading and processing time.
    surface_load_time = duration(load_end - load_start).count();

    // surface.update();
    fit_view();
    print_surface_info();
    // compute_topology_and_geometry();

    surface_should_update = true;

    app().info(format("Sucessfully loaded surface mesh from file.\nfile = '{}'",
                      path.string()));

  } catch (exception& e) {
    app().error(
        format("Failed to load surface mesh from file.\n{}\nfile = '{}'",
               e.what(), path.string()));
    return;
  }
}

void viewer::async_load_surface(const filesystem::path& path) {
  if (surface_load_task.valid()) {
    app().error(format(
        "Failed to start asynchronous loading of surface mesh from file.\nIt "
        "seems another surface mesh is already loading.\n file = '{}'",
        path.string()));
    return;
  }
  surface_load_task =
      async(launch::async, [this, &path] { load_surface(path); });
  app().info(format(
      "Started to asynchronously load surface mesh from file.\nfile = '{}'",
      path.string()));
}

void viewer::handle_surface_load_task() {
  if (!surface_load_task.valid()) return;
  if (future_status::ready != surface_load_task.wait_for(0s)) {
    // cout << "." << flush;
    return;
  }

  // info("Sucessfully finished to asynchronously load surface mesh.");

  surface_load_task = {};
}

void viewer::fit_view() {
  const auto box = aabb_from(surface);
  origin = box.origin();
  bounding_radius = box.radius();

  // cout << "bounding radius = " << bounding_radius << endl;

  radius = bounding_radius / tan(0.5f * camera.vfov());
  camera.set_near_and_far(1e-4f * radius, 2 * radius);
  view_should_update = true;
}

void viewer::print_surface_info() {
  app().info(format(  //
      "load time = {:6.3f}s\n"
      "process time = {:6.3f}s\n"
      "vertices = {}\n"
      "faces = {}\n",
      surface_load_time, surface_process_time, surface.vertices.size(),
      surface.faces.size()));
}

}  // namespace ensketch::sandbox
