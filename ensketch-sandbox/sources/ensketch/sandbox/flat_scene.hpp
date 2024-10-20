#pragma once
#include <ensketch/sandbox/aabb.hpp>
#include <ensketch/sandbox/defaults.hpp>

namespace ensketch::sandbox {

struct flat_scene {
  using size_type = uint32;

  struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
  };

  struct face : std::array<size_type, 3> {};

  struct index_span {
    size_type first{};
    size_type last{};
  };

  struct mesh {
    std::string name{};
    index_span vertices{};
    index_span faces{};
  };

  struct hierarchy {
    struct node {
      std::string name{};
      glm::mat4 transform{};
      size_type parent{};
      index_span children{};
    };
    std::vector<node> nodes{};
    std::vector<size_type> children{};
  };

  std::string name{};
  std::vector<vertex> vertices{};
  std::vector<face> faces{};
  std::vector<mesh> meshes{};
  struct hierarchy hierarchy {};
};

/// Constructor Extension for AABB
/// Get the bounding box around a polyhedral surface.
///
auto aabb_from(const flat_scene& scene) noexcept -> aabb3;

auto flat_scene_from_file(const std::filesystem::path& path) -> flat_scene;

}  // namespace ensketch::sandbox
