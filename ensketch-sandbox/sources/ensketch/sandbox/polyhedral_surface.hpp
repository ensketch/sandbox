#pragma once
#include <ensketch/opengl/opengl.hpp>
#include <ensketch/sandbox/aabb.hpp>
#include <ensketch/sandbox/stl_surface.hpp>
#include <ensketch/sandbox/utility.hpp>

namespace ensketch::sandbox {

struct polyhedral_surface {
  using size_type = uint32;

  using real = float32;
  static constexpr uint32 invalid = -1;

  struct vertex {
    vec3 position;
    vec3 normal;
  };
  using vertex_id = uint32;

  struct face : array<vertex_id, 3> {};
  using face_id = uint32;

  struct edge : array<vertex_id, 2> {
    struct info {
      face_id face;
    };

    struct hasher {
      auto operator()(const edge& e) const noexcept -> size_t {
        return (size_t(e[0]) << 7) ^ size_t(e[1]);
      }
    };
  };

  void generate_edges() {
    edges.clear();
    for (size_t i = 0; i < faces.size(); ++i) {
      const auto& f = faces[i];
      edges[edge{f[0], f[1]}].face = i;
      edges[edge{f[1], f[2]}].face = i;
      edges[edge{f[2], f[0]}].face = i;
    }

    neighbor_count.assign(vertices.size(), 0);
    mean_edge_length.assign(vertices.size(), 0);
    max_edge_length.assign(vertices.size(), 0);

    for (const auto& [e, _] : edges) {
      const auto vid = e[0];
      const auto nid = e[1];
      ++neighbor_count[vid];
      const auto l = distance(vertices[vid].position, vertices[nid].position);
      mean_edge_length[vid] += l;

      max_edge_length[vid] = std::max(max_edge_length[vid], l);
      max_edge_length[nid] = std::max(max_edge_length[nid], l);
    }
    for (size_t i = 0; i < vertices.size(); ++i)
      mean_edge_length[i] /= neighbor_count[i];
  }

  vector<vertex> vertices{};
  vector<face> faces{};
  unordered_map<edge, edge::info, edge::hasher> edges{};

  vector<size_t> neighbor_count{};
  vector<float> mean_edge_length{};
  vector<float> max_edge_length{};
};

auto polyhedral_surface_from(const stl_surface& data) -> polyhedral_surface;

auto polyhedral_surface_from(const filesystem::path& path)
    -> polyhedral_surface;

/// Constructor Extension for AABB
/// Get the bounding box around a polyhedral surface.
///
auto aabb_from(const polyhedral_surface& surface) noexcept -> aabb3;

struct scene : polyhedral_surface {
  auto host() noexcept -> polyhedral_surface& { return *this; }
  auto host() const noexcept -> const polyhedral_surface& { return *this; }

  void setup() noexcept {
    device_handle.bind();
    device_vertices.bind();
    device_faces.bind();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                          (void*)offsetof(vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                          (void*)offsetof(vertex, normal));
  }

  void update() noexcept {
    device_vertices.allocate_and_initialize(vertices);
    device_faces.allocate_and_initialize(faces);
  }

  void render() const noexcept {
    device_handle.bind();
    device_faces.bind();
    glDrawElements(GL_TRIANGLES, 3 * faces.size(), GL_UNSIGNED_INT, 0);
  }

  opengl::vertex_array device_handle{};
  opengl::vertex_buffer device_vertices{};
  opengl::element_buffer device_faces{};
};

}  // namespace ensketch::sandbox
