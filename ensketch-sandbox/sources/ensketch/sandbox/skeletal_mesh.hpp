#pragma once
#include <ensketch/opengl/opengl.hpp>
#include <ensketch/sandbox/aabb.hpp>
#include <ensketch/sandbox/defaults.hpp>

namespace ensketch::sandbox {

struct skeletal_mesh {
  using size_type = uint32;

  using real = float32;
  static constexpr uint32 invalid = -1;

  using bone_id = uint32;
  static constexpr bone_id max_bone_influence = 4;

  struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
    // bone indexes which will influence this vertex
    bone_id bone_ids[max_bone_influence];
    // weights from each bone
    float bone_weights[max_bone_influence];
  };

  using vertex_id = uint32;

  struct face : std::array<vertex_id, 3> {};
  using face_id = uint32;

  struct bone {
    glm::mat4 offset{};
  };

  std::vector<vertex> vertices{};
  std::vector<face> faces{};

  // std::map<std::string, bone_info> bones{};
  std::map<std::string, bone_id> bone_ids{};
  std::vector<bone> bones{};
};

/// Constructor Extension for AABB
/// Get the bounding box around a polyhedral surface.
///
auto aabb_from(const skeletal_mesh& mesh) noexcept -> aabb3;

auto skeletal_mesh_from_file(const std::filesystem::path& path)
    -> skeletal_mesh;

}  // namespace ensketch::sandbox
