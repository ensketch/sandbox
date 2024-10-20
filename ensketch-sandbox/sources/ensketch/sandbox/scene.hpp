#pragma once
#include <ensketch/sandbox/aabb.hpp>
#include <ensketch/sandbox/defaults.hpp>

namespace ensketch::sandbox {

struct scene_index {
  using size_type = uint32;
  static constexpr size_type invalid = -1;

  constexpr scene_index() noexcept = default;
  constexpr scene_index(std::integral auto id) noexcept
      : data{static_cast<size_type>(id)} {}

  constexpr operator bool() noexcept { return data != invalid; }

  // constexpr operator size_type() noexcept { return data; }

  constexpr decltype(auto) operator=(std::integral auto id) noexcept {
    data = static_cast<size_type>(id);
    return *this;
  }

  size_type data = invalid;
};

using scene_bone_index = scene_index;

struct scene_vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

using scene_vertex_index = scene_index;

struct scene_face : std::array<scene_vertex_index, 3> {};

// struct scene_bone {
//   std::string name{};
//   glm::mat4 offset{};
// };

struct scene_mesh {
  std::string name{};
  std::vector<scene_vertex> vertices{};
  std::vector<scene_face> faces{};
};

template <typename type>
struct scene_animation_key {
  float64 time{};
  type data{};
};
using scene_animation_position_key = scene_animation_key<glm::vec3>;
using scene_animation_rotation_key = scene_animation_key<glm::quat>;
using scene_animation_scaling_key = scene_animation_key<glm::vec3>;

struct scene_animation_channel {
  std::string node_name{};
  std::vector<scene_animation_position_key> positions{};
  std::vector<scene_animation_rotation_key> rotations{};
  std::vector<scene_animation_scaling_key> scalings{};

  auto position(float64 time) const -> glm::mat4;
  auto rotation(float64 time) const -> glm::mat4;
  auto scaling(float64 time) const -> glm::mat4;
  auto transform(float64 time) const -> glm::mat4;
};

struct scene_animation {
  std::string name{};
  float64 duration{};
  float64 ticks{};
  std::vector<scene_animation_channel> channels{};
};

using scene_node_index = scene_index;
using scene_mesh_index = scene_index;

// struct scene_node2 {
//   // const scene_node* parent{};
//   std::vector<scene_node2> children{};
//   std::string name{};
//   glm::mat4 transform{};
//   std::vector<scene_mesh_index> meshes{};
// };

// struct scene_node {
//   std::string name{};
//   glm::mat4 transform{};
//   scene_node_index parent{};
//   std::pair<scene_index, scene_index> children_offset{};
//   std::pair<scene_index, scene_index> meshes_offset{};
// };

struct scene_bone_weight {
  uint32 vid{};
  float32 weight{};
};

struct scene_bone_entry {
  uint32 mid{};
  glm::mat4 offset{};
  std::vector<scene_bone_weight> weights{};
};

struct scene_node {
  // Node Data
  std::string name{};
  glm::mat4 transform{};
  std::vector<uint32> meshes{};
  std::vector<scene_bone_entry> bone_entries{};
  // Connectivity Data
  scene_node* parent{};
  std::list<scene_node> children{};
};

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

struct scene_skeleton_animation {};

struct scene_skeleton {
  std::vector<uint32> parents{};
  std::vector<scene_skeleton_bone> bones{};
  std::vector<scene_node*> nodes{};
  std::vector<scene_skeleton_mesh_weight_data> weights{};

  std::map<std::string_view, uint32> bone_name_map{};

  // std::vector<scene_animation> animations{};

  auto global_transforms(const scene_animation& animation,
                         float64 time) -> std::vector<glm::mat4> {
    std::vector<glm::mat4> result{};
    result.reserve(bones.size());
    for (size_t bid = 0; bid < bones.size(); ++bid)
      result.push_back(bones[bid].transform);

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

// struct scene_hierarchy {
// std::vector<scene_node> nodes{};
// std::vector<scene_node_index> children{};
// std::vector<scene_mesh_index> meshes{};
// };

struct scene {
  using vertex = scene_vertex;

  std::string name{};

  scene_node root{};
  std::map<std::string_view, scene_node&> node_name_map{};

  std::vector<scene_mesh> meshes{};
  std::vector<scene_animation> animations{};
  // scene_hierarchy hierarchy{};
  // scene_node2 root{};

  scene_skeleton skeleton{};
};

// struct scene {
//   using size_type = uint32;
//   static constexpr size_type invalid = -1;
//   static constexpr size_type max_bone_influence = 4;

//   struct ref {
//     static constexpr size_type invalid = -1;
//     constexpr operator bool() noexcept { return data != invalid; }
//     constexpr operator size_type() noexcept { return data; }
//     constexpr decltype(auto) operator=(size_type id) noexcept {
//       data = id;
//       return *this;
//     }
//     size_type data = invalid;
//   };

//   using real = float32;
//   using vertex_id = size_type;
//   using face_id = size_type;
//   using bone_id = size_type;

//   using bone_ref = ref;

//   struct vertex {
//     glm::vec3 position;
//     glm::vec3 normal;
//     bone_ref bone_ids[max_bone_influence]{};
//     float bone_weights[max_bone_influence]{};
//   };

//   struct face : std::array<vertex_id, 3> {};

//   struct bone {
//     std::string name{};
//     glm::mat4 offset{};
//   };

//   struct mesh {
//     std::string name{};
//     std::vector<vertex> vertices{};
//     std::vector<face> faces{};
//     std::vector<bone> bones{};
//   };

//   template <typename type>
//   struct key {
//     float64 time{};
//     type data{};
//   };
//   using position_key = key<glm::vec3>;
//   using rotation_key = key<glm::quat>;
//   using scaling_key = key<glm::vec3>;

//   struct anim_node {
//     std::vector<position_key> positions{};
//     std::vector<rotation_key> rotations{};
//     std::vector<scaling_key> scalings{};

//     auto position(float64 time) -> glm::mat4 {
//       if (positions.empty()) return glm::mat4{1.0f};
//       if (positions.size() == 1)
//         return glm::translate(glm::mat4{1.0f}, positions[0].data);

//       size_type i = 0;
//       for (; i < positions.size() - 1; ++i)
//         if (time < positions[i + 1].time) break;

//       auto t1 = positions[i].time;
//       auto t2 = positions[i + 1].time;
//       auto t = (time - t1) / (t2 - t1);

//       const auto p = glm::mix(positions[i].data, positions[i + 1].data, t);
//       return glm::translate(glm::mat4{1.0f}, p);
//     }

//     auto rotation(float64 time) -> glm::mat4 {
//       if (rotations.empty()) return glm::mat4{1.0f};
//       if (rotations.size() == 1)
//         return glm::toMat4(glm::normalize(rotations[0].data));

//       size_type i = 0;
//       for (; i < rotations.size() - 1; ++i)
//         if (time < rotations[i + 1].time) break;

//       auto t1 = rotations[i].time;
//       auto t2 = rotations[i + 1].time;
//       auto t = (time - t1) / (t2 - t1);

//       const auto p =
//           glm::slerp(rotations[i].data, rotations[i + 1].data, float32(t));
//       return glm::toMat4(glm::normalize(p));
//     }

//     auto scaling(float64 time) -> glm::mat4 {
//       if (scalings.empty()) return glm::mat4{1.0f};
//       if (scalings.size() == 1)
//         return glm::scale(glm::mat4{1.0f}, scalings[0].data);

//       size_type i = 0;
//       for (; i < scalings.size() - 1; ++i)
//         if (time < scalings[i + 1].time) break;

//       auto t1 = scalings[i].time;
//       auto t2 = scalings[i + 1].time;
//       auto t = (time - t1) / (t2 - t1);

//       const auto p = glm::mix(scalings[i].data, scalings[i + 1].data, t);
//       return glm::scale(glm::mat4{1.0f}, p);
//     }

//     auto transform(float64 time) -> glm::mat4 {
//       return position(time) * rotation(time) * scaling(time);
//     }
//   };

//   struct animation {
//     std::string name{};
//     float64 duration{};
//     std::vector<anim_node> channels{};
//   };

//   std::string name{};
//   std::vector<mesh> meshes{};
// };

/// Constructor Extension for AABB
/// Get the bounding box around a polyhedral surface.
///
auto aabb_from(const scene& mesh) noexcept -> aabb3;

auto scene_from_file(const std::filesystem::path& path) -> scene;

}  // namespace ensketch::sandbox
