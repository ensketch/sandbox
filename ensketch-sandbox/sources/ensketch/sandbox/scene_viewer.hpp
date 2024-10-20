#pragma once
#include <ensketch/sandbox/basic_viewer.hpp>
#include <ensketch/sandbox/flat_scene.hpp>
#include <ensketch/sandbox/scene.hpp>

namespace ensketch::sandbox {

///
///
class scene_viewer_state : public basic_viewer_state {
 public:
  using base = basic_viewer_state;

  scene_viewer_state() {
    const auto vs = opengl::vertex_shader{"#version 460 core\n",  //
                                          R"##(
uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;

layout (std430, binding = 0) readonly buffer bone_weight_offsets {
  uint offsets[];
};
struct bone_weight {
  uint bid;
  float weight;
};
layout (std430, binding = 1) readonly buffer bone_weight_data {
  bone_weight weights[];
};
layout (std430, binding = 2) readonly buffer bone_transforms {
  mat4 transforms[];
};

out vec3 position;
out vec3 normal;
out vec3 color;

void main() {
  mat4 bone_transform = mat4(0.0);
  for (uint i = offsets[gl_VertexID]; i < offsets[gl_VertexID + 1]; ++i)
    bone_transform += weights[i].weight * transforms[weights[i].bid];

  gl_Position = projection * view * bone_transform * vec4(p, 1.0);

  position = vec3(view * vec4(p, 1.0));
  normal = vec3(view * vec4(n, 0.0));

  // color = vec3(float(offsets[gl_VertexID]) / weights.length());
  // color = vec3(float(offsets[gl_VertexID + 1] - offsets[gl_VertexID]) / 4);

  // color = vec3(0.0);
  // for (uint i = offsets[gl_VertexID]; i < offsets[gl_VertexID + 1]; ++i)
  //   color[i - offsets[gl_VertexID]] = weights[i].weight;

  // color = vec3(0.0);
  // for (uint i = offsets[gl_VertexID]; i < offsets[gl_VertexID + 1]; ++i)
  //   color[i - offsets[gl_VertexID]] = float(weights[i].bid) / transforms.length();

  color = vec3(1.0);
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
in vec3 color[];

out vec3 pos;
out vec3 nor;
out vec3 vnor;
out vec3 col;
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

  gl_PrimitiveID = gl_PrimitiveIDIn;

  edge_distance = vec3(ha, 0, 0);
  nor = n;
  vnor = normal[0];
  pos = position[0];
  col = color[0];
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, hb, 0);
  nor = n;
  vnor = normal[1];
  pos = position[1];
  col = color[1];
  gl_Position = gl_in[1].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, 0, hc);
  nor = n;
  vnor = normal[2];
  pos = position[2];
  col = color[2];
  gl_Position = gl_in[2].gl_Position;
  EmitVertex();

  EndPrimitive();
}
)##"};

    const auto fs = opengl::fragment_shader{R"##(
#version 460 core

uniform bool wireframe = false;
uniform bool use_face_normal = false;

in vec3 pos;
in vec3 nor;
in vec3 vnor;
in vec3 col;
noperspective in vec3 edge_distance;

layout (location = 0) out vec4 frag_color;

void main() {
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

  vec4 light_color = vec4(vec3(light) * col, alpha);

  // Mix both color values.

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

  void pretty_print_node(const scene_node& node,
                         const std::string& prefix,
                         const std::string& child_prefix) {
    log::text(std::format("{}{}", prefix, node.name));

    for (auto mid : node.meshes)
      log::text(std::format("{}↳ {}", child_prefix, surface.meshes[mid].name));

    for (auto [mid, offset, weights] : node.bone_entries) {
      log::text(std::format("{}↣ {} ({})", child_prefix,
                            surface.meshes[mid].name, weights.size()));
      // log::text(std::format("{}  {},{},{},{}", child_prefix, offset[0][0],
      //                       offset[0][1], offset[0][2], offset[0][3]));
      // log::text(std::format("{}  {},{},{},{}", child_prefix, offset[1][0],
      //                       offset[1][1], offset[1][2], offset[1][3]));
      // log::text(std::format("{}  {},{},{},{}", child_prefix, offset[2][0],
      //                       offset[2][1], offset[2][2], offset[2][3]));
      // log::text(std::format("{}  {},{},{},{}", child_prefix, offset[3][0],
      //                       offset[3][1], offset[3][2], offset[3][3]));
    }

    auto it = node.children.begin();
    if (it == node.children.end()) return;
    auto next = it;
    ++next;
    for (; next != node.children.end(); ++next) {
      pretty_print_node(*it, child_prefix + "├─", child_prefix + "│ ");
      it = next;
    }
    pretty_print_node(*it, child_prefix + "└─", child_prefix + "  ");
  }

  void print_node(const scene_node& node, size_t indent) {
    log::text(fmt::format("{:>{}}{}", " ", indent, node.name));
    for (const auto& child : node.children) print_node(child, indent + 1);
  }

  // void print_node(size_t index, size_t indent) {
  //   auto& node = surface.hierarchy.nodes[index];
  //   auto [p, q] = node.children;
  //   log::text(fmt::format("{:>{}}{} ({},{})", " ", indent, node.name, p, q));
  //   for (size_t i = p; i < q; ++i)
  //     print_node(surface.hierarchy.children[i], indent + 1);
  // }

  // void print_hierarchy() {
  //   log::text("Hierarchy:");
  //   print_node(0, 0);
  // }

  // void print_animations() {
  //   log::text("Animations:");
  //   for (auto& anim : surface.animations) {
  //     log::text(anim.name);
  //     for (auto& channel : anim.channels)
  //       log::text(fmt::format("  {}", channel.node_name));
  //   }
  //   log::text("");
  // }

  void print_scene_info() {
    log::text("\nScene Information:");
    log::text(std::format("name = {}", surface.name));
    // log::text(std::format("vertices = {}", surface.vertices.size()));
    // log::text(std::format("faces = {}", surface.faces.size()));
    // log::text("---");

    log::text("Meshes:");
    for (const auto& mesh : surface.meshes) {
      log::text(std::format("  name = {}", mesh.name));
      log::text(std::format("  vertices = {}", mesh.vertices.size()));
      log::text(std::format("  faces = {}", mesh.faces.size()));
      //   log::text(std::format("  vertices = [{},{})",  //
      //                         mesh.vertices.first, mesh.vertices.last));
      //   log::text(std::format("  faces = [{},{})",  //
      //                         mesh.faces.first, mesh.faces.last));
      log::text("  ---");
    }

    log::text("Hierarchy:");
    // log::text(std::format("  nodes = {}", surface.hierarchy.nodes.size()));
    // log::text("  ---");
    // // for (auto& node : surface.hierarchy.nodes) {
    // //   log::text(std::format("  {} ({},{})", node.name,
    // node.children.first,
    // //                         node.children.last));
    // // }
    // // log::text("  ---");
    // // for (size_t i = 0; auto index : surface.hierarchy.children) {
    // //   log::text(std::format("  children[{}] = {}", i, index));
    // //   ++i;
    // // }
    // // log::text("  ---");
    // print_node(surface.root, 2);
    pretty_print_node(surface.root, " ─", "  ");
    log::text("  ---");

    log::text("Animations:");
    for (auto& anim : surface.animations) {
      log::text(std::format("  name = {}", anim.name));
      log::text(std::format("  time = {}", anim.duration));
      log::text(std::format("  tick = {}", anim.ticks));
      log::text("  Channels:");
      for (auto& channel : anim.channels)
        log::text(fmt::format("    {} ({},{},{})",  //
                              channel.node_name,    //
                              channel.positions.size(),
                              channel.rotations.size(),
                              channel.scalings.size()));
      log::text("  ---");
    }

    log::text("Skeleton:");
    for (size_t i = 0; i < surface.skeleton.bones.size(); ++i) {
      log::text(fmt::format("  {:>4}: {} ↣ {}", i,
                            surface.skeleton.nodes[i]->name,
                            surface.skeleton.parents[i]));
    }
    log::text("  ---");
    // for (size_t mid = 0; auto& wdata : surface.skeleton.weights) {
    //   log::text(std::format("  mesh = {}", surface.meshes[mid].name));
    //   for (size_t vid = 0;
    //        vid < std::min(10ul, surface.meshes[mid].vertices.size()); ++vid)
    //        {
    //     log::text(std::format("  {}: {}", vid, wdata.offsets[vid]));
    //     for (size_t k = wdata.offsets[vid]; k < wdata.offsets[vid + 1]; ++k)
    //     {
    //       log::text(std::format(
    //           "      {}: {}",
    //           surface.skeleton.nodes[wdata.data[k].bone]->name,
    //           wdata.data[k].weight));
    //     }
    //   }
    //   ++mid;
    //   log::text("  ---");
    // }

    log::text("");
  }

  void load_surface(const std::filesystem::path& path) {
    try {
      surface = scene_from_file(path);

      print_scene_info();

      // log::text("Meshes:");
      // for (auto& mesh : surface.meshes) {
      //   log::text(mesh.name);
      //   for (auto& bone : mesh.bones) log::text(bone.name);
      //   log::text("");
      // }

      // log::text("Nodes:");
      // for (auto& node : surface.hierarchy.nodes) log::text(node.name);
      // print_hierarchy();
      // print_animations();

      fit_view_to_surface();
      // vertices.allocate_and_initialize(surface.vertices);
      // faces.allocate_and_initialize(surface.faces);

      // device_mesh.va.bind();
      // device_mesh.vertices.bind();
      // device_mesh.faces.bind();
      // glEnableVertexAttribArray(0);
      // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
      //                       sizeof(flat_scene::vertex),
      //                       (void*)offsetof(flat_scene::vertex, position));
      // glEnableVertexAttribArray(1);
      // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
      //                       sizeof(flat_scene::vertex),
      //                       (void*)offsetof(flat_scene::vertex, normal));
      // device_mesh.vertices.allocate_and_initialize(surface.vertices);
      // device_mesh.faces.allocate_and_initialize(surface.faces);

      device_meshes.resize(surface.meshes.size());
      for (size_t i = 0; i < surface.meshes.size(); ++i) {
        device_meshes[i].va.bind();
        device_meshes[i].vertices.bind();
        device_meshes[i].faces.bind();

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(scene::vertex),
                              (void*)offsetof(scene::vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(scene::vertex),
                              (void*)offsetof(scene::vertex, normal));

        device_meshes[i].vertices.allocate_and_initialize(
            surface.meshes[i].vertices);
        device_meshes[i].faces.allocate_and_initialize(surface.meshes[i].faces);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                     device_meshes[i].bone_weight_offsets.id());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                         device_meshes[i].bone_weight_offsets.id());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,
                     device_meshes[i].bone_weight_data.id());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
                         device_meshes[i].bone_weight_data.id());

        device_meshes[i].bone_weight_offsets.allocate_and_initialize(
            surface.skeleton.weights[i].offsets);
        device_meshes[i].bone_weight_data.allocate_and_initialize(
            surface.skeleton.weights[i].data);
      }
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, bone_transforms.id());
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bone_transforms.id());

      // auto tmp = surface.skeleton.global_transforms();
      // bone_transforms.allocate_and_initialize(tmp);
      // for (auto& x : tmp) {
      //   x = transpose(x);
      //   log::text(
      //       std::format("{},{},{},{}", x[0][0], x[0][1], x[0][2], x[0][3]));
      //   log::text(
      //       std::format("{},{},{},{}", x[1][0], x[1][1], x[1][2], x[1][3]));
      //   log::text(
      //       std::format("{},{},{},{}", x[2][0], x[2][1], x[2][2], x[2][3]));
      //   log::text(
      //       std::format("{},{},{},{}", x[3][0], x[3][1], x[3][2], x[3][3]));
      //   log::text("---");
      // }

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

    shader.use();
    // device_mesh.va.bind();
    // device_mesh.faces.bind();
    // glDrawElements(GL_TRIANGLES, 3 * surface.faces.size(), GL_UNSIGNED_INT,
    // 0);

    if (not surface.animations.empty()) {
      const auto current = std::chrono::high_resolution_clock::now();
      const auto time = std::fmod(
          std::chrono::duration<float64>(current - start).count(),
          surface.animations[0].duration / surface.animations[0].ticks);
      auto tmp =
          surface.skeleton.global_transforms(surface.animations[0], time);
      bone_transforms.allocate_and_initialize(tmp);
    }

    for (size_t i = 0; i < surface.meshes.size(); ++i) {
      device_meshes[i].va.bind();
      device_meshes[i].faces.bind();
      // glBindBuffer(GL_SHADER_STORAGE_BUFFER,
      //              device_meshes[i].bone_weight_offsets.id());
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                       device_meshes[i].bone_weight_offsets.id());
      // glBindBuffer(GL_SHADER_STORAGE_BUFFER,
      //              device_meshes[i].bone_weight_data.id());
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
                       device_meshes[i].bone_weight_data.id());
      glDrawElements(GL_TRIANGLES, 3 * surface.meshes[i].faces.size(),
                     GL_UNSIGNED_INT, 0);
    }
  }

  void set_wireframe(bool value) { shader.set("wireframe", value); }

  void use_face_normal(bool value) { shader.set("use_face_normal", value); }

 protected:
  // polyhedral_surface surface{};
  scene surface{};
  // flat_scene surface{};
  float bounding_radius;

  std::chrono::time_point<std::chrono::high_resolution_clock> start =
      std::chrono::high_resolution_clock::now();

  opengl::shader_program shader{};
  struct opengl_mesh {
    opengl::vertex_array va{};
    opengl::vertex_buffer vertices{};
    opengl::element_buffer faces{};

    opengl::shader_storage_buffer bone_weight_offsets{};
    opengl::shader_storage_buffer bone_weight_data{};
  };
  opengl::shader_storage_buffer bone_transforms{};
  // opengl_mesh device_mesh{};
  std::vector<opengl_mesh> device_meshes{};
};

///
///
template <typename derived>
struct scene_viewer_api : basic_viewer_api<derived> {
  using base = basic_viewer_api<derived>;
  using base::self;
  using state_type = scene_viewer_state;

  void load_surface(std::string_view path) {
    self().async_invoke_and_discard(
        [path = std::string{path}](state_type& state) {
          state.load_surface(path);
        });
  }

  void set_wireframe(bool value) {
    self().async_invoke_and_discard(
        [value](state_type& state) { state.set_wireframe(value); });
  }

  void use_face_normal(bool value) {
    self().async_invoke_and_discard(
        [value](state_type& state) { state.use_face_normal(value); });
  }
};

}  // namespace ensketch::sandbox
