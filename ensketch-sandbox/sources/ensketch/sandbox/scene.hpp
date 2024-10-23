#pragma once
#include <ensketch/sandbox/aabb.hpp>
#include <ensketch/sandbox/defaults.hpp>

namespace ensketch::sandbox {

struct scene_skeleton_bone {
  glm::mat4 offset;
  glm::mat4 transform;
};

struct scene_skeleton_mesh_weight {
  uint32 bone{};
  float32 weight{};
};

struct scene_skeleton_mesh_weight_data {
  std::vector<uint32> offsets{};
  std::vector<scene_skeleton_mesh_weight> data{};
};

struct scene {
  struct mesh {
    struct vertex {
      glm::vec3 position;
      glm::vec3 normal;
    };

    struct face : std::array<uint32, 3> {};

    std::string name{};
    std::vector<vertex> vertices{};
    std::vector<face> faces{};
  };

  struct node {
    struct bone_entry {
      struct weight_entry {
        uint32 vertex{};
        float32 weight{};
      };

      // Data
      uint32 mesh{};
      std::vector<weight_entry> weights{};
    };

    /// Connectivity Data
    node* parent{};  // Use bare pointer type for easy no-overhead referencing.
    std::list<node> children{};  // Use `std::list` for easy modification.

    /// Node Data
    uint32 index{};
    std::string name{};
    glm::mat4 offset{1.0f};
    glm::mat4 transform{1.0f};
    std::vector<uint32> meshes{};
    std::vector<bone_entry> bone_entries{};
  };

  struct animation {
    struct channel {
      template <typename type>
      struct key {
        float64 time{};
        type data{};
      };
      using position_key = key<glm::vec3>;
      using rotation_key = key<glm::quat>;
      using scaling_key = key<glm::vec3>;

      std::string node_name{};
      std::vector<position_key> positions{};
      std::vector<rotation_key> rotations{};
      std::vector<scaling_key> scalings{};

      auto position(float64 time) const -> glm::mat4;
      auto rotation(float64 time) const -> glm::mat4;
      auto scaling(float64 time) const -> glm::mat4;
      auto transform(float64 time) const -> glm::mat4;
    };

    std::string name{};
    float64 duration{};
    float64 ticks{};
    std::vector<channel> channels{};
  };

  struct flat_skeleton {
    std::vector<uint32> parents{};
    std::vector<scene_skeleton_bone> bones{};
    std::vector<node*> nodes{};
    std::vector<scene_skeleton_mesh_weight_data> weights{};

    std::map<std::string_view, uint32> bone_name_map{};

    // std::vector<scene_animation> animations{};

    auto global_transforms(const scene::animation& animation,
                           float64 time) -> std::vector<glm::mat4> {
      std::vector<glm::mat4> result{};
      result.reserve(bones.size());
      for (size_t bid = 0; bid < bones.size(); ++bid)
        result.push_back(glm::mat4(1.0f));
      // result.push_back(bones[bid].transform);

      for (const auto& channel : animation.channels) {
        if (auto it = bone_name_map.find(channel.node_name);
            it != bone_name_map.end()) {
          const auto bid = it->second;
          // const auto bid = bone_name_map.at(channel.node_name);
          result[bid] = channel.transform(time * animation.ticks);
        }
      }

      for (size_t bid = 0; bid < bones.size(); ++bid)
        if (parents[bid] < bones.size())
          result[bid] = result[parents[bid]] * result[bid];

      for (size_t bid = 0; bid < bones.size(); ++bid)
        result[bid] *= bones[bid].offset;
      // result[bid] = translate(glm::mat4(1.0f), glm::vec3(1.0f + bid));

      return result;
    }
  };

  std::string name{};
  std::vector<mesh> meshes{};
  node root{};
  uint32 node_count{};
  std::map<std::string_view, node&> node_name_map{};
  std::vector<animation> animations{};

  flat_skeleton skeleton{};
};

void traverse(scene::node& node, auto&& f);
void traverse(const scene::node& node, auto&& f);

/// Constructor Extension for AABB
/// Get the bounding box around a polyhedral surface.
///
auto aabb_from(const scene& mesh) noexcept -> aabb3;

auto scene_from_file(const std::filesystem::path& path) -> scene;

}  // namespace ensketch::sandbox

#include <ensketch/sandbox/scene.ipp>
