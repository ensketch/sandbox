#pragma once
#include <ensketch/sandbox/scene.hpp>

namespace ensketch::sandbox {

struct triangle_mesh {
  using vertex = scene::mesh::vertex;
  using face = scene::mesh::face;
  struct group_offset {
    uint32 vertex{};
    uint32 face{};
  };

  std::vector<vertex> vertices{};
  std::vector<face> faces{};
  std::vector<group_offset> group_offsets{};
};

auto triangle_mesh_from(const scene& in) -> triangle_mesh;

struct skeleton {
  auto size() const noexcept -> uint32 { return parents.size(); }
  std::vector<uint32> parents{};
  std::vector<glm::mat4> transforms{};
  std::vector<glm::mat4> offsets{};
};

auto skeleton_from(const scene& in) -> skeleton;

// compressed by vertices
struct weight_matrix {
  struct entry {
    uint32 index{};
    float32 weight{};
  };
  std::vector<uint32> offsets{};
  std::vector<entry> entries{};
};

struct skinned_mesh : triangle_mesh {
  struct animation {
    struct channel {
      template <typename type>
      struct key {
        float32 time{};
        type data{};
      };
      using position_key = key<glm::vec3>;
      using rotation_key = key<glm::quat>;
      using scaling_key = key<glm::vec3>;

      uint32 index;
      std::vector<position_key> positions{};
      std::vector<rotation_key> rotations{};
      std::vector<scaling_key> scalings{};

      auto position(float32 time) const -> glm::mat4;
      auto rotation(float32 time) const -> glm::mat4;
      auto scaling(float32 time) const -> glm::mat4;
      auto transform(float32 time) const -> glm::mat4;
    };

    float32 duration{};
    float32 ticks{};
    std::vector<channel> channels{};
  };

  skeleton bones{};
  weight_matrix weights{};
  std::vector<animation> animations{};
};

auto skinned_mesh_from(const scene& in) -> skinned_mesh;

void load_global_transforms(const skinned_mesh& mesh, auto&& out) {
  auto& bones = mesh.bones;
  assert(std::ranges::size(out) == bones.size());
  std::ranges::copy(bones.transforms, std::ranges::begin(out));
  for (size_t bid = 1; bid < bones.size(); ++bid)
    out[bid] = out[bones.parents[bid]] * out[bid];
  for (size_t bid = 0; bid < bones.size(); ++bid)
    out[bid] *= bones.offsets[bid];
}

inline auto global_transforms(const skinned_mesh& mesh)
    -> std::vector<glm::mat4> {
  std::vector<glm::mat4> result;
  result.resize(mesh.bones.size());
  load_global_transforms(mesh, result);
  return result;
}

void load_animation_transforms(const skinned_mesh& mesh,
                               size_t aid,
                               float32 time,
                               auto&& out) {
  assert(aid < mesh.animations.size());
  auto& animation = mesh.animations[aid];
  auto& bones = mesh.bones;
  assert(std::ranges::size(out) == bones.size());

  std::ranges::fill(out, glm::mat4{1.0f});
  for (size_t i = 0; i < animation.channels.size(); ++i) {
    auto& channel = animation.channels[i];
    out[channel.index] = channel.transform(time * animation.ticks);
  }
  for (size_t bid = 1; bid < bones.size(); ++bid)
    out[bid] = out[bones.parents[bid]] * out[bid];
  for (size_t bid = 0; bid < bones.size(); ++bid)
    out[bid] *= bones.offsets[bid];
}

inline auto animation_transforms(const skinned_mesh& mesh,
                                 size_t aid,
                                 float32 time) -> std::vector<glm::mat4> {
  std::vector<glm::mat4> result;
  result.resize(mesh.bones.size());
  load_animation_transforms(mesh, aid, time, result);
  return result;
}

}  // namespace ensketch::sandbox
