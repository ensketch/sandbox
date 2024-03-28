#include <ensketch/sandbox/viewer.hpp>
//
#include <ensketch/sandbox/application.hpp>
#include <ensketch/sandbox/ray_tracer.hpp>
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

// OpenGL will be set to 4.6.
//
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
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(10.0f);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  device = device_storage{};

  const auto vs = opengl::vertex_shader{"#version 460 core\n",  //
                                        R"##(
uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;

out vec3 position;
out vec3 normal;

void main() {
  gl_Position = projection * view * vec4(p, 1.0);
  position = vec3(view * vec4(p, 1.0));
  normal = vec3(view * vec4(n, 0.0));
}
)##"};

  const auto gs = opengl::geometry_shader{R"##(
#version 460 core

uniform mat4 view;
uniform mat4 viewport;

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 position[];
in vec3 normal[];

out vec3 pos;
out vec3 nor;
out vec3 vnor;
noperspective out vec3 edge_distance;

void main(){
  vec3 p0 = vec3(viewport * (gl_in[0].gl_Position /
                             gl_in[0].gl_Position.w));
  vec3 p1 = vec3(viewport * (gl_in[1].gl_Position /
                             gl_in[1].gl_Position.w));
  vec3 p2 = vec3(viewport * (gl_in[2].gl_Position /
                             gl_in[2].gl_Position.w));

  float a = length(p1 - p2);
  float b = length(p2 - p0);
  float c = length(p1 - p0);

  vec3 n = normalize(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));

  float alpha = acos((b * b + c * c - a * a) / (2.0 * b * c));
  float beta  = acos((a * a + c * c - b * b) / (2.0 * a * c));

  float ha = abs(c * sin(beta));
  float hb = abs(c * sin(alpha));
  float hc = abs(b * sin(alpha));

  edge_distance = vec3(ha, 0, 0);
  nor = n;
  vnor = normal[0];
  pos = position[0];
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, hb, 0);
  nor = n;
  vnor = normal[1];
  pos = position[1];
  gl_Position = gl_in[1].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, 0, hc);
  nor = n;
  vnor = normal[2];
  pos = position[2];
  gl_Position = gl_in[2].gl_Position;
  EmitVertex();

  EndPrimitive();
}
)##"};

  const auto fs = opengl::fragment_shader{R"##(
#version 460 core

// uniform bool lighting = true;

in vec3 pos;
in vec3 nor;
in vec3 vnor;
noperspective in vec3 edge_distance;

layout (location = 0) out vec4 frag_color;

void main() {
  // vec3 n = normalize(normal);
  // vec3 view_dir = vec3(0.0, 0.0, 1.0);
  // vec3 light_dir = view_dir;
  // float d = max(dot(light_dir, n), 0.0);
  // vec3 color = vec3(vec2(0.2 + 1.0 * pow(d,1000) + 0.75 * pow(d,0.2)),1.0);
  // frag_color = vec4(color, 1.0);

  // Compute distance from edges.
  float d = min(edge_distance.x, edge_distance.y);
  d = min(d, edge_distance.z);
  float line_width = 0.01;
  float line_delta = 1.0;
  float alpha = 1.0;
  vec4 line_color = vec4(vec3(0.5), alpha);
  float mix_value =
      smoothstep(line_width - line_delta, line_width + line_delta, d);
  // float mix_value = 1.0;
  // Compute viewer shading.
  float s = abs(normalize(nor).z);
  // float s = abs(normalize(vnor).z);
  float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);
  // float light = 0.2 + 0.75 * pow(s, 0.2);
  vec4 light_color = vec4(vec3(light), alpha);
  // Mix both color values.
  // vec4 color = vec4(vec3(colormap(hea)), alpha);
  //light_color = mix(color, light_color, 0.0);
  // if (!lighting) light_color = color;
  frag_color = mix(line_color, light_color, mix_value);
  // if (mix_value > 0.9) discard;
  //   frag_color = (1 - mix_value) * line_color;
}
)##"};

  if (!vs) {
    app().error(vs.info_log());
    app().close_viewer();
    return;
  }

  if (!gs) {
    app().error(gs.info_log());
    app().close_viewer();
    return;
  }

  if (!fs) {
    app().error(fs.info_log());
    app().close_viewer();
    return;
  }

  device->shader.attach(vs);
  device->shader.attach(gs);
  device->shader.attach(fs);
  device->shader.link();

  if (!device->shader.linked()) {
    app().error(device->shader.info_log());
    app().close_viewer();
    return;
  }

  const auto point_vs = opengl::vertex_shader{R"##(
#version 460 core

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;

void main(){
  gl_Position = projection * view * vec4(p, 1.0);
}
)##"};

  const auto point_fs = opengl::fragment_shader{R"##(
#version 460 core

layout (location = 0) out vec4 frag_color;

void main() {
  float alpha = 1.0;
  vec2 tmp = 2.0 * gl_PointCoord - 1.0;
  float r = dot(tmp, tmp);
  float delta = fwidth(0.5 * r);
  alpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, r);
  frag_color = vec4(0.1, 0.5, 0.9, alpha);
}
)##"};

  if (!point_vs) {
    app().error(point_vs.info_log());
    app().close_viewer();
    return;
  }

  if (!point_fs) {
    app().error(point_fs.info_log());
    app().close_viewer();
    return;
  }

  device->point_shader.attach(point_vs);
  device->point_shader.attach(point_fs);
  device->point_shader.link();

  if (!device->point_shader.linked()) {
    app().error(device->point_shader.info_log());
    app().close_viewer();
    return;
  }

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

  const auto surface_vertex_curve_vs = opengl::vertex_shader{R"##(
#version 460 core

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;

void main(){
  gl_Position = projection * view * vec4(p, 1.0);
}
)##"};

  const auto surface_vertex_curve_fs = opengl::fragment_shader{R"##(
#version 460 core

layout (location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(0.1, 0.5, 0.9, 1.0);
}
)##"};

  if (!surface_vertex_curve_vs) {
    app().error(surface_vertex_curve_vs.info_log());
    app().close_viewer();
    return;
  }

  if (!surface_vertex_curve_fs) {
    app().error(surface_vertex_curve_fs.info_log());
    app().close_viewer();
    return;
  }

  device->surface_vertex_curve_shader.attach(surface_vertex_curve_vs);
  device->surface_vertex_curve_shader.attach(surface_vertex_curve_fs);
  device->surface_vertex_curve_shader.link();

  if (!device->surface_vertex_curve_shader.linked()) {
    app().error(device->surface_vertex_curve_shader.info_log());
    app().close_viewer();
    return;
  }

  const auto mouse_curve_vs = opengl::vertex_shader{R"##(
#version 460 core

uniform float screen_width;
uniform float screen_height;

layout (location = 0) in vec2 p;

void main(){
  const float x = 2.0 * p.x / screen_width - 1.0;
  const float y = 1.0 - 2.0 * p.y / screen_height;
  gl_Position = vec4(x, y, 0.0, 1.0);
}
)##"};

  const auto mouse_curve_fs = opengl::fragment_shader{R"##(
#version 460 core

layout (location = 0) out vec4 frag_color;

void main() {
  float alpha = 1.0;
  vec2 tmp = 2.0 * gl_PointCoord - 1.0;
  float r = dot(tmp, tmp);
  float delta = fwidth(0.5 * r);
  alpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, r);
  frag_color = vec4(0.1, 0.5, 0.9, alpha);
}
)##"};

  if (!mouse_curve_vs) {
    app().error(mouse_curve_vs.info_log());
    app().close_viewer();
    return;
  }

  if (!mouse_curve_fs) {
    app().error(mouse_curve_fs.info_log());
    app().close_viewer();
    return;
  }

  device->mouse_curve_shader.attach(mouse_curve_vs);
  device->mouse_curve_shader.attach(mouse_curve_fs);
  device->mouse_curve_shader.link();

  if (!device->mouse_curve_shader.linked()) {
    app().error(device->mouse_curve_shader.info_log());
    app().close_viewer();
    return;
  }

  device->mouse_curve_va.bind();
  device->mouse_curve_data.bind();
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
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
        case sf::Mouse::Right:
          select_vertex(mouse_pos.x, mouse_pos.y);
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
        case sf::Keyboard::Space:
          mouse_curve_recording = !mouse_curve_recording;
          break;
        case sf::Keyboard::M:
          reset_mouse_curve();
          mouse_curve_recording = false;
          break;
        case sf::Keyboard::P:
          project_mouse_curve();
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

  if (mouse_curve_recording) record_mouse_curve();
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

  if (device) {
    device->shader.try_set("projection", camera.projection_matrix());
    device->shader.try_set("view", camera.view_matrix());
    device->shader.try_set("viewport", camera.viewport_matrix());

    device->point_shader.try_set("projection", camera.projection_matrix());
    device->point_shader.try_set("view", camera.view_matrix());
    device->point_shader.try_set("viewport", camera.viewport_matrix());

    device->surface_vertex_curve_shader.try_set("projection",
                                                camera.projection_matrix());
    device->surface_vertex_curve_shader.try_set("view", camera.view_matrix());
    device->surface_vertex_curve_shader.try_set("viewport",
                                                camera.viewport_matrix());

    device->mouse_curve_shader.set("screen_width",
                                   float(camera.screen_width()));
    device->mouse_curve_shader.set("screen_height",
                                   float(camera.screen_height()));
  }
}

void viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glDepthFunc(GL_LEQUAL);

  device->va.bind();
  device->faces.bind();
  device->shader.use();
  glDrawElements(GL_TRIANGLES, 3 * surface.faces.size(), GL_UNSIGNED_INT, 0);
  // glDrawArrays(GL_TRIANGLES, 0, 3);

  glDepthFunc(GL_ALWAYS);

  if (selected_vertex != polyhedral_surface::invalid) {
    device->va.bind();
    device->selected_vertices.bind();
    device->point_shader.use();
    glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, 0);
  }

  if (!mouse_curve.empty()) {
    device->mouse_curve_va.bind();
    device->mouse_curve_data.bind();
    device->mouse_curve_shader.use();
    glDrawArrays(GL_POINTS, 0, mouse_curve.size());
  }

  if (!surface_vertex_curve.empty()) {
    device->va.bind();
    device->surface_vertex_curve_data.bind();
    device->surface_vertex_curve_shader.use();
    glDrawElements(GL_LINE_STRIP, surface_vertex_curve.size(), GL_UNSIGNED_INT,
                   0);
  }

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
    surface.generate_edges();

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

auto viewer::mouse_to_vertex(float x, float y) noexcept
    -> polyhedral_surface::vertex_id {
  const auto r = primary_ray(camera, x, y);
  const auto p = intersection(r, surface);
  if (!p) return polyhedral_surface::invalid;

  const auto& f = surface.faces[p.f];
  const auto w = real(1) - p.u - p.v;

  const auto position = surface.vertices[f[0]].position * w +
                        surface.vertices[f[1]].position * p.u +
                        surface.vertices[f[2]].position * p.v;

  const auto l0 = length(position - surface.vertices[f[0]].position);
  const auto l1 = length(position - surface.vertices[f[1]].position);
  const auto l2 = length(position - surface.vertices[f[2]].position);

  if (l0 <= l1) {
    if (l0 <= l2)
      return f[0];
    else
      return f[2];
  } else {
    if (l1 <= l2)
      return f[1];
    else
      return f[2];
  }

  return polyhedral_surface::invalid;
}

void viewer::select_vertex(float x, float y) noexcept {
  selected_vertex = mouse_to_vertex(x, y);
  if (selected_vertex == polyhedral_surface::invalid) return;
  app().info(format("Vertex ID = {}", selected_vertex));
  if (device)
    device->selected_vertices.allocate_and_initialize(selected_vertex);
}

void viewer::reset_mouse_curve() noexcept {
  mouse_curve.clear();
  // if (device) device->mouse_curve_data.clear();
}

void viewer::record_mouse_curve() noexcept {
  mouse_curve.push_back(vec2{mouse_pos.x, mouse_pos.y});
  if (device) device->mouse_curve_data.allocate_and_initialize(mouse_curve);
}

void viewer::project_mouse_curve() {
  surface_vertex_curve.clear();

  for (auto& m : mouse_curve) {
    const auto x = mouse_to_vertex(m.x, m.y);

    if (x == polyhedral_surface::invalid) continue;

    if (surface_vertex_curve.empty()) {
      surface_vertex_curve.push_back(x);
      continue;
    }

    const auto p = surface_vertex_curve.back();
    if (x == p) continue;

    if (surface_vertex_curve.size() < 2) {
      surface_vertex_curve.push_back(x);
      continue;
    }

    const auto q = surface_vertex_curve[surface_vertex_curve.size() - 2];
    if (x == q) {
      surface_vertex_curve.pop_back();
      continue;
    }

    if (surface.edges.contains({q, x}) || surface.edges.contains({x, q})) {
      surface_vertex_curve.pop_back();
      surface_vertex_curve.push_back(x);
      continue;
    }

    surface_vertex_curve.push_back(x);
  }

  if (device)
    device->surface_vertex_curve_data.allocate_and_initialize(
        surface_vertex_curve);
}

}  // namespace ensketch::sandbox
