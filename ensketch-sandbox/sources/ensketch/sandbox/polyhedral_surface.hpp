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

inline auto bipartition_from(const polyhedral_surface& surface,
                             const vector<polyhedral_surface::vertex_id>& curve,
                             bool closed = true) -> vector<float> {
  const auto throw_error = [] {
    throw runtime_error(
        "Failed to generate surface bi-partition from given surface vertex "
        "curve.");
  };

  if (curve.empty()) throw_error();

  // constexpr uint8 r_flag = 0b01;
  // constexpr uint8 l_flag = 0b10;
  // constexpr uint8 both = 0b11;

  vector<float> face_mask{};
  face_mask.assign(surface.faces.size(), 0.0f);

  vector<polyhedral_surface::face_id> face_stack{};

  for (size_t i = 1; i < curve.size(); ++i) {
    const auto p = curve[i - 1];
    const auto q = curve[i];

    const auto fr = surface.edges.at(polyhedral_surface::edge{p, q}).face;
    face_mask[fr] = 1.0f;
    // face_mask[fr] |= r_flag;

    const auto fl = surface.edges.at(polyhedral_surface::edge{q, p}).face;
    face_mask[fl] = -1.0f;
    // face_mask[fl] |= l_flag;

    // if (face_mask[fr] == both) throw_error();
    // if (face_mask[fl] == both) throw_error();

    // face_stack.push_back(fr);
    // face_stack.push_back(fl);
  }
  if (closed) {
    const auto p = curve.back();
    const auto q = curve.front();

    const auto fr = surface.edges.at(polyhedral_surface::edge{p, q}).face;
    face_mask[fr] = 1.0f;
    // face_mask[fr] |= r_flag;

    const auto fl = surface.edges.at(polyhedral_surface::edge{q, p}).face;
    face_mask[fl] = -1.0f;
    // face_mask[fl] |= l_flag;

    // if (face_mask[fr] == both) throw_error();
    // if (face_mask[fl] == both) throw_error();

    // face_stack.push_back(fr);
    // face_stack.push_back(fl);
  }

  for (size_t i = 0; i < surface.faces.size(); ++i) {
    if (face_mask[i]) continue;
    const auto face = surface.faces[i];
    const auto n0 =
        surface.edges.at(polyhedral_surface::edge{face[1], face[0]}).face;
    const auto n1 =
        surface.edges.at(polyhedral_surface::edge{face[2], face[1]}).face;
    const auto n2 =
        surface.edges.at(polyhedral_surface::edge{face[0], face[2]}).face;
    if ((face_mask[n0] != 0.0f) || (face_mask[n1] != 0.0f) ||
        (face_mask[n2] != 0.0f))
      face_stack.push_back(i);
  }

  while (!face_stack.empty()) {
    const auto f = face_stack.back();
    face_stack.pop_back();

    const auto face = surface.faces[f];
    const auto n0 =
        surface.edges.at(polyhedral_surface::edge{face[1], face[0]}).face;
    const auto n1 =
        surface.edges.at(polyhedral_surface::edge{face[2], face[1]}).face;
    const auto n2 =
        surface.edges.at(polyhedral_surface::edge{face[0], face[2]}).face;

    bool left = false;
    bool right = false;

    if (face_mask[n0] == 0.0f)
      face_stack.push_back(n0);
    else {
      if (face_mask[n0] < 0.0f) left = true;
      if (face_mask[n0] > 0.0f) right = true;
    }

    if (face_mask[n1] == 0.0f)
      face_stack.push_back(n1);
    else {
      if (face_mask[n1] < 0.0f) left = true;
      if (face_mask[n1] > 0.0f) right = true;
    }

    if (face_mask[n2] == 0.0f)
      face_stack.push_back(n2);
    else {
      if (face_mask[n2] < 0.0f) left = true;
      if (face_mask[n2] > 0.0f) right = true;
    }

    if (left && right) throw_error();
    if (left) face_mask[f] = -1.0f;
    if (right) face_mask[f] = 1.0f;
  }

  return face_mask;
}

}  // namespace ensketch::sandbox
