#pragma once
#include <ensketch/sandbox/basic_viewer.hpp>
#include <ensketch/sandbox/polyhedral_surface.hpp>

namespace ensketch::sandbox {

///
///
class surface_viewer_state : public basic_viewer_state {
 public:
  using base = basic_viewer_state;

  surface_viewer_state() {
    va.bind();
    vertices.bind();
    faces.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(polyhedral_surface::vertex),
        (void*)offsetof(polyhedral_surface::vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(polyhedral_surface::vertex),
                          (void*)offsetof(polyhedral_surface::vertex, normal));

    const auto vs = opengl::vertex_shader{"#version 460 core\n",  //
                                          R"##(
uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in float f;

out vec3 position;
out vec3 normal;
out float field;

void main() {
  gl_Position = projection * view * vec4(p, 1.0);
  position = vec3(view * vec4(p, 1.0));
  normal = vec3(view * vec4(n, 0.0));
  field = f;
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
in float field[];

out vec3 pos;
out vec3 nor;
out vec3 vnor;
noperspective out vec3 edge_distance;
out float phi;

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

  gl_PrimitiveID = gl_PrimitiveIDIn;

  edge_distance = vec3(ha, 0, 0);
  nor = n;
  vnor = normal[0];
  pos = position[0];
  phi = field[0];
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, hb, 0);
  nor = n;
  vnor = normal[1];
  pos = position[1];
  phi = field[1];
  gl_Position = gl_in[1].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, 0, hc);
  nor = n;
  vnor = normal[2];
  pos = position[2];
  phi = field[2];
  gl_Position = gl_in[2].gl_Position;
  EmitVertex();

  EndPrimitive();
}
)##"};

    const auto fs = opengl::fragment_shader{R"##(
#version 460 core

// uniform bool lighting = true;

uniform bool wireframe = false;
uniform bool use_face_normal = false;

in vec3 pos;
in vec3 nor;
in vec3 vnor;
noperspective in vec3 edge_distance;
in float phi;

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

  float s = abs(normalize(vnor).z);
  if (use_face_normal)
    s = abs(normalize(nor).z);

  float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);

  // float light = 0.2 + 0.75 * pow(s, 0.2);

  vec4 light_color = vec4(vec3(light), alpha);

  // Mix both color values.

  float transition = 0.5;

  // light_color = mix(vec4(0.0, 0.0, 0.0, 1.0), light_color, smoothstep(phi, -10.0, 10.0));
  // if (!lighting) light_color = color;

  if (wireframe)
    frag_color = mix(line_color, light_color, mix_value);
  else
    frag_color = light_color;
}
)##"};

    if (!vs) {
      log::error(vs.info_log());
      sandbox::quit();
      return;
    }

    if (!gs) {
      log::error(gs.info_log());
      sandbox::quit();
      return;
    }

    if (!fs) {
      log::error(fs.info_log());
      sandbox::quit();
      return;
    }

    shader.attach(vs);
    shader.attach(gs);
    shader.attach(fs);
    shader.link();

    if (!shader.linked()) {
      log::error(shader.info_log());
      sandbox::quit();
      return;
    }
  }

  void load_surface(const std::filesystem::path& path) {
    try {
      surface = polyhedral_surface_from(path);
      surface.generate_edges();
      fit_view_to_surface();
      vertices.allocate_and_initialize(surface.vertices);
      faces.allocate_and_initialize(surface.faces);

      log::info(
          format("Sucessfully loaded surface mesh from file.\nfile = '{}'",
                 path.string()));

    } catch (exception& e) {
      log::error(
          format("Failed to load surface mesh from file.\n{}\nfile = '{}'",
                 e.what(), path.string()));
      return;
    }
  }

  void fit_view_to_surface() {
    const auto box = aabb_from(surface);
    origin = box.origin();
    bounding_radius = box.radius();

    // cout << "bounding radius = " << bounding_radius << endl;

    radius = bounding_radius / tan(0.5f * camera.vfov());
    camera.set_near_and_far(1e-4f * radius, 2 * radius);
    view_should_update = true;
  }

  void render() {
    base::render();

    shader.try_set("projection", camera.projection_matrix());
    shader.try_set("view", camera.view_matrix());
    shader.try_set("viewport", camera.viewport_matrix());

    va.bind();
    faces.bind();
    shader.use();
    glDrawElements(GL_TRIANGLES, 3 * surface.faces.size(), GL_UNSIGNED_INT, 0);
  }

 protected:
  polyhedral_surface surface{};
  float bounding_radius;
  opengl::shader_program shader{};
  opengl::vertex_array va{};
  opengl::vertex_buffer vertices{};
  opengl::element_buffer faces{};
};

///
///
template <typename derived>
struct surface_viewer_api : basic_viewer_api<derived> {
  using base = basic_viewer_api<derived>;
  using base::self;
  using state_type = surface_viewer_state;

  void load_surface(std::string_view path) {
    self().async_invoke_and_discard(
        [path = std::string{path}](state_type& state) {
          state.load_surface(path);
        });
  }
};

}  // namespace ensketch::sandbox
