#include <ensketch/sandbox/log.hpp>
#include <ensketch/sandbox/skinned_mesh.hpp>

namespace ensketch::sandbox {

auto triangle_mesh_from(const scene& in) -> triangle_mesh {
  triangle_mesh out{};

  out.group_offsets.resize(in.meshes.size() + 1);
  for (size_t mid = 0; mid < in.meshes.size(); ++mid) {
    const auto& mesh = in.meshes[mid];
    out.group_offsets[mid + 1].vertex =
        out.group_offsets[mid].vertex + mesh.vertices.size();
    out.group_offsets[mid + 1].face =
        out.group_offsets[mid].face + mesh.faces.size();
  }

  out.vertices.resize(out.group_offsets.back().vertex);
  out.faces.resize(out.group_offsets.back().face);

  for (size_t mid = 0; mid < in.meshes.size(); ++mid) {
    const auto& mesh = in.meshes[mid];
    const auto voffset = out.group_offsets[mid].vertex;
    const auto foffset = out.group_offsets[mid].face;

    for (size_t vid = 0; vid < mesh.vertices.size(); ++vid)
      out.vertices[voffset + vid] = mesh.vertices[vid];

    for (size_t fid = 0; fid < mesh.faces.size(); ++fid)
      for (size_t k = 0; auto vid : mesh.faces[fid]) {
        out.faces[foffset + fid][k] = voffset + vid;
        ++k;
      }
  }

  return out;
}

auto skeleton_from(const scene& in) -> skeleton {
  skeleton out{};

  out.parents.resize(in.node_count);
  out.transforms.resize(in.node_count);
  out.offsets.resize(in.node_count);

  traverse(in.root, [&out](auto& node) {
    out.parents[node.index] = (node.parent) ? (node.parent->index) : (0);
    out.transforms[node.index] = node.transform;
    out.offsets[node.index] = node.offset;
  });

  return out;
}

auto skinned_mesh::animation::channel::position(float32 time) const
    -> glm::mat4 {
  if (positions.empty()) return glm::mat4{1.0f};
  if (positions.size() == 1)
    return glm::translate(glm::mat4{1.0f}, positions[0].data);

  size_t i = 0;
  for (; i < positions.size() - 1; ++i)
    if (time < positions[i + 1].time) break;

  auto t1 = positions[i].time;
  auto t2 = positions[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p = glm::mix(positions[i].data, positions[i + 1].data, t);
  return glm::translate(glm::mat4{1.0f}, p);
}

auto skinned_mesh::animation::channel::rotation(float32 time) const
    -> glm::mat4 {
  if (rotations.empty()) return glm::mat4{1.0f};
  if (rotations.size() == 1)
    return glm::toMat4(glm::normalize(rotations[0].data));

  size_t i = 0;
  for (; i < rotations.size() - 1; ++i)
    if (time < rotations[i + 1].time) break;

  auto t1 = rotations[i].time;
  auto t2 = rotations[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p =
      glm::slerp(rotations[i].data, rotations[i + 1].data, float32(t));
  return glm::toMat4(glm::normalize(p));
}

auto skinned_mesh::animation::channel::scaling(float32 time) const
    -> glm::mat4 {
  if (scalings.empty()) return glm::mat4{1.0f};
  if (scalings.size() == 1)
    return glm::scale(glm::mat4{1.0f}, scalings[0].data);

  size_t i = 0;
  for (; i < scalings.size() - 1; ++i)
    if (time < scalings[i + 1].time) break;

  auto t1 = scalings[i].time;
  auto t2 = scalings[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p = glm::mix(scalings[i].data, scalings[i + 1].data, t);
  return glm::scale(glm::mat4{1.0f}, p);
}

auto skinned_mesh::animation::channel::transform(float32 time) const
    -> glm::mat4 {
  return position(time) * rotation(time) * scaling(time);
}

auto skinned_mesh_from(const scene& in) -> skinned_mesh {
  skinned_mesh out{triangle_mesh_from(in), skeleton_from(in)};

  auto& offsets = out.weights.offsets;
  auto& entries = out.weights.entries;
  offsets.assign(1 + out.vertices.size(), 0);

  // Count weight entries for each vertex.
  traverse(in.root, [&](auto& node) {
    for (auto& [mid, weights] : node.bone_entries) {
      const auto voffset = out.group_offsets[mid].vertex;
      for (auto& [vid, weight] : weights)  //
        ++offsets[voffset + vid];
    }
  });

  // Accumulate and allocate.
  for (size_t i = 1; i < offsets.size(); ++i)  //
    offsets[i] += offsets[i - 1];
  entries.resize(offsets.back());

  // Assign all weight entries.
  traverse(in.root, [&](auto& node) {
    for (auto& [mid, weights] : node.bone_entries) {
      const auto voffset = out.group_offsets[mid].vertex;
      for (auto& [vid, weight] : weights)  //
        entries[--offsets[voffset + vid]] = {node.index, weight};
    }
  });

  // Load animations
  out.animations.resize(in.animations.size());
  for (size_t i = 0; i < in.animations.size(); ++i) {
    out.animations[i].duration = in.animations[i].duration;
    out.animations[i].ticks = in.animations[i].ticks;
    out.animations[i].channels.resize(in.animations[i].channels.size());
    for (size_t j = 0; const auto& channel : in.animations[i].channels) {
      auto it = in.node_name_map.find(channel.node_name);
      if (it == in.node_name_map.end()) {
        log::warn(std::format("unknown node '{}' in channel {} of animation {}",
                              channel.node_name, j, i));
        continue;
      }
      out.animations[i].channels[j].index = it->second.index;

      out.animations[i].channels[j].positions.resize(channel.positions.size());
      for (size_t k = 0; k < channel.positions.size(); ++k)
        out.animations[i].channels[j].positions[k] = {
            channel.positions[k].time, channel.positions[k].data};

      out.animations[i].channels[j].rotations.resize(channel.rotations.size());
      for (size_t k = 0; k < channel.rotations.size(); ++k)
        out.animations[i].channels[j].rotations[k] = {
            channel.rotations[k].time, channel.rotations[k].data};

      out.animations[i].channels[j].scalings.resize(channel.scalings.size());
      for (size_t k = 0; k < channel.scalings.size(); ++k)
        out.animations[i].channels[j].scalings[k] =  //
            {channel.scalings[k].time, channel.scalings[k].data};

      ++j;
    }
  }

  return out;
}

}  // namespace ensketch::sandbox
