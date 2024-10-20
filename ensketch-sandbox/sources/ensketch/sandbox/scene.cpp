#include <ensketch/sandbox/scene.hpp>
//
#include <ensketch/sandbox/log.hpp>
//
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

namespace ensketch::sandbox {

using size_type = scene_index::size_type;

static auto vec3_from(const aiVector3D& v) noexcept -> glm::vec3 {
  return {v.x, v.y, v.z};
};

static auto quat_from(const aiQuaternion& q) noexcept -> glm::quat {
  return {q.w, q.x, q.y, q.z};
}

static auto mat4_from(const aiMatrix4x4& from) noexcept -> glm::mat4 {
  return {from.a1, from.a2, from.a3, from.a4,  //
          from.b1, from.b2, from.b3, from.b4,  //
          from.c1, from.c2, from.c3, from.c4,  //
          from.d1, from.d2, from.d3, from.d4};
};

static auto scene_mesh_from(const aiMesh* in) -> scene_mesh {
  scene_mesh out{};

  // Name
  out.name = in->mName.C_Str();

  // Vertices
  out.vertices.reserve(in->mNumVertices);
  for (size_type vid = 0; vid < in->mNumVertices; ++vid) {
    out.vertices.push_back(
        scene_vertex{.position = vec3_from(in->mVertices[vid]),
                     .normal = vec3_from(in->mNormals[vid])});
  }

  // Faces
  out.faces.reserve(in->mNumFaces);
  for (size_type fid = 0; fid < in->mNumFaces; ++fid) {
    auto face = in->mFaces[fid];
    // All faces need to be triangles.
    // So, use a simple triangulation of polygons.
    for (size_type k = 2; k < face.mNumIndices; ++k) {
      out.faces.push_back({face.mIndices[0],      //
                           face.mIndices[k - 1],  //
                           face.mIndices[k]});
    }
  }

  // Bones
  // out.bones.reserve(in->mNumBones);
  // log::text(std::format(""));
  // for (size_type bid = 0; bid < in->mNumBones; ++bid) {
  //   auto bone = in->mBones[bid];
  //   out.bones.push_back({.name = bone->mName.C_Str(),
  //                        .offset = mat4_from(bone->mOffsetMatrix)});

  //   // for (size_type wid = 0; wid < bone->mNumWeights; ++wid) {
  //   //   auto vid = bone->mWeights[wid].mVertexId;
  //   //   float weight = bone->mWeights[wid].mWeight;

  //   //   for (size_type i = 0; i < scene_vertex::max_bone_influence; ++i) {
  //   //     if (out.vertices[vid].bone_ids[i]) continue;
  //   //     out.vertices[vid].bone_ids[i] = bid;
  //   //     out.vertices[vid].bone_weights[i] = weight;
  //   //     return out;
  //   //   }
  //   // }
  // }
  return out;
}

static void load_node(const aiNode* in,
                      scene_node& out,
                      scene_node* parent = nullptr) {
  out.name = in->mName.C_Str();
  out.transform = mat4_from(in->mTransformation);

  // Affected Mesh
  out.meshes.reserve(in->mNumMeshes);
  for (size_t i = 0; i < in->mNumMeshes; ++i)
    out.meshes.push_back(in->mMeshes[i]);

  // Connectivity
  out.parent = parent;
  for (size_type i = 0; i < in->mNumChildren; ++i)
    load_node(in->mChildren[i], out.children.emplace_back(), &out);
}

static void traverse(scene_node& node, auto&& f) {
  std::invoke(f, node);
  for (auto& child : node.children) traverse(child, f);
}

static void update_node_name_map(scene& s) {
  s.node_name_map.clear();
  traverse(s.root, [&s](scene_node& node) {
    s.node_name_map.emplace(node.name, node);
  });
}

static void load_bone_entries(const aiScene* in, scene& out) {
  for (size_t mid = 0; mid < in->mNumMeshes; ++mid) {
    auto mesh = in->mMeshes[mid];
    for (size_t bid = 0; bid < mesh->mNumBones; ++bid) {
      auto bone = mesh->mBones[bid];
      auto& node = out.node_name_map.at(bone->mName.C_Str());
      auto& entry =
          node.bone_entries.emplace_back(mid, mat4_from(bone->mOffsetMatrix));
      entry.weights.reserve(bone->mNumWeights);
      for (size_t wid = 0; wid < bone->mNumWeights; ++wid) {
        const auto vid = bone->mWeights[wid].mVertexId;
        const float weight = bone->mWeights[wid].mWeight;
        entry.weights.emplace_back(vid, weight);
      }
    }
  }
}

// static auto traverse_and_assign(const aiNode* in,
//                                 scene_hierarchy& out,
//                                 scene_node_index parent = {})
//     -> scene_node_index {
//   const auto current = scene_node_index{out.nodes.size()};

//   out.nodes.push_back(scene_node{
//       .name = in->mName.C_Str(),
//       .transform = mat4_from(in->mTransformation),
//       .parent = parent,
//       .children_offset = {scene_index{out.children.size()},
//                           scene_index{out.children.size() +
//                           in->mNumChildren}},
//       .meshes_offset = {scene_index{out.meshes.size()},
//                         scene_index{out.meshes.size() + in->mNumMeshes}},
//   });

//   // `out.meshes.size()` and `out.children.size()` determine the state.
//   // These values need to be updated before recursively processing the
//   children.

//   // Mesh Indices
//   for (size_type i = 0; i < in->mNumMeshes; ++i)
//     out.meshes.emplace_back(in->mMeshes[i]);

//   // Children Indices
//   const auto offset = out.children.size();
//   out.children.resize(out.children.size() + in->mNumChildren);
//   for (size_type i = 0; i < in->mNumChildren; ++i)
//     out.children[offset + i] =
//         traverse_and_assign(in->mChildren[i], out, current);

//   return current;
// }

// static auto scene_hierarchy_from(const aiNode* root) -> scene_hierarchy {
//   // The given node must be the root node of the scene.
//   assert(root->mParent == nullptr);
//   scene_hierarchy out{};
//   // Use a helper function to recursively construct the node hierarchy.
//   traverse_and_assign(root, out);
//   return out;
// }

// static auto scene_node_from(const aiNode* in/*,
//                             const scene_node* parent = nullptr*/) ->
//                             scene_node2 {
//   scene_node2 out{
//       // .parent = parent,
//       .name = in->mName.C_Str(),
//       .transform = mat4_from(in->mTransformation),
//   };

//   // Meshes
//   out.meshes.reserve(in->mNumMeshes);
//   for (size_type i = 0; i < in->mNumMeshes; ++i)
//     out.meshes.emplace_back(in->mMeshes[i]);

//   // Children
//   out.children.reserve(in->mNumChildren);
//   for (size_type i = 0; i < in->mNumChildren; ++i)
//     out.children.push_back(scene_node_from(in->mChildren[i]));

//   return out;
// }

static auto scene_animation_channel_from(const aiNodeAnim* in)
    -> scene_animation_channel {
  scene_animation_channel out{};
  // Node Name
  out.node_name = in->mNodeName.C_Str();
  // Position Keys
  out.positions.reserve(in->mNumPositionKeys);
  for (size_type i = 0; i < in->mNumPositionKeys; ++i)
    out.positions.push_back(
        {in->mPositionKeys[i].mTime, vec3_from(in->mPositionKeys[i].mValue)});
  // Rotation Keys
  out.rotations.reserve(in->mNumRotationKeys);
  for (size_type i = 0; i < in->mNumRotationKeys; ++i)
    out.rotations.push_back(
        {in->mRotationKeys[i].mTime, quat_from(in->mRotationKeys[i].mValue)});
  // Scaling Keys
  out.scalings.reserve(in->mNumScalingKeys);
  for (size_type i = 0; i < in->mNumScalingKeys; ++i)
    out.scalings.push_back(
        {in->mScalingKeys[i].mTime, vec3_from(in->mScalingKeys[i].mValue)});
  return out;
}

auto scene_animation_channel::position(float64 time) -> glm::mat4 {
  if (positions.empty()) return glm::mat4{1.0f};
  if (positions.size() == 1)
    return glm::translate(glm::mat4{1.0f}, positions[0].data);

  size_type i = 0;
  for (; i < positions.size() - 1; ++i)
    if (time < positions[i + 1].time) break;

  auto t1 = positions[i].time;
  auto t2 = positions[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p = glm::mix(positions[i].data, positions[i + 1].data, t);
  return glm::translate(glm::mat4{1.0f}, p);
}

auto scene_animation_channel::rotation(float64 time) -> glm::mat4 {
  if (rotations.empty()) return glm::mat4{1.0f};
  if (rotations.size() == 1)
    return glm::toMat4(glm::normalize(rotations[0].data));

  size_type i = 0;
  for (; i < rotations.size() - 1; ++i)
    if (time < rotations[i + 1].time) break;

  auto t1 = rotations[i].time;
  auto t2 = rotations[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p =
      glm::slerp(rotations[i].data, rotations[i + 1].data, float32(t));
  return glm::toMat4(glm::normalize(p));
}

auto scene_animation_channel::scaling(float64 time) -> glm::mat4 {
  if (scalings.empty()) return glm::mat4{1.0f};
  if (scalings.size() == 1)
    return glm::scale(glm::mat4{1.0f}, scalings[0].data);

  size_type i = 0;
  for (; i < scalings.size() - 1; ++i)
    if (time < scalings[i + 1].time) break;

  auto t1 = scalings[i].time;
  auto t2 = scalings[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p = glm::mix(scalings[i].data, scalings[i + 1].data, t);
  return glm::scale(glm::mat4{1.0f}, p);
}

auto scene_animation_channel::transform(float64 time) -> glm::mat4 {
  return position(time) * rotation(time) * scaling(time);
}

static auto scene_animation_from(const aiAnimation* in) -> scene_animation {
  scene_animation out{};
  // Name
  out.name = in->mName.C_Str();
  // Duration
  out.duration = in->mDuration;
  out.ticks = in->mTicksPerSecond;
  // Bone Animation Channels
  out.channels.reserve(in->mNumChannels);
  for (size_type i = 0; i < in->mNumChannels; ++i)
    out.channels.push_back(scene_animation_channel_from(in->mChannels[i]));
  return out;
}

auto aabb_from(const scene& s) noexcept -> aabb3 {
  aabb3 result{};
  for (const auto& mesh : s.meshes)
    for (const auto& v : mesh.vertices)
      result = sandbox::aabb_from(result, v.position);
  return result;
}

// static auto mesh_from(const aiMesh* in) -> scene::mesh {
//   using size_type = scene::size_type;
//   scene::mesh out{};

//   // Vertices
//   out.vertices.reserve(in->mNumVertices);
//   for (size_type vid = 0; vid < in->mNumVertices; ++vid) {
//     out.vertices.push_back(
//         scene::vertex{.position = vec3_from(in->mVertices[vid]),
//                       .normal = vec3_from(in->mNormals[vid])});
//   }

//   // Faces
//   out.faces.reserve(in->mNumFaces);
//   for (size_type fid = 0; fid < in->mNumFaces; ++fid) {
//     auto face = in->mFaces[fid];
//     // All faces need to be triangles.
//     // So, use a simple triangulation of polygons.
//     for (size_type k = 2; k < face.mNumIndices; ++k) {
//       out.faces.push_back({face.mIndices[0],      //
//                            face.mIndices[k - 1],  //
//                            face.mIndices[k]});
//     }
//   }

//   // Bones
//   out.bones.reserve(in->mNumBones);
//   for (size_type bid = 0; bid < in->mNumBones; bid) {
//     auto bone = in->mBones[bid];
//     out.bones.push_back({.name = bone->mName.C_Str(),
//                          .offset = mat4_from(bone->mOffsetMatrix)});

//     for (size_type wid = 0; wid < bone->mNumWeights; ++wid) {
//       auto vid = bone->mWeights[wid].mVertexId;
//       float weight = bone->mWeights[wid].mWeight;

//       for (size_type i = 0; i < scene::max_bone_influence; ++i) {
//         if (out.vertices[vid].bone_ids[i]) continue;
//         out.vertices[vid].bone_ids[i] = bid;
//         out.vertices[vid].bone_weights[i] = weight;
//       }
//     }
//   }
//   return out;
// }

// static auto anim_node_from(const aiNodeAnim* in) -> scene::anim_node {
//   scene::anim_node out{};
//   // Position Keys
//   out.positions.reserve(in->mNumPositionKeys);
//   for (size_type i = 0; i < in->mNumPositionKeys; ++i)
//     out.positions.push_back(
//         {in->mPositionKeys[i].mTime,
//         vec3_from(in->mPositionKeys[i].mValue)});
//   // Rotation Keys
//   out.rotations.reserve(in->mNumRotationKeys);
//   for (size_type i = 0; i < in->mNumRotationKeys; ++i)
//     out.rotations.push_back(
//         {in->mRotationKeys[i].mTime,
//         quat_from(in->mRotationKeys[i].mValue)});
//   // Scaling Keys
//   out.scalings.reserve(in->mNumScalingKeys);
//   for (size_type i = 0; i < in->mNumScalingKeys; ++i)
//     out.scalings.push_back(
//         {in->mScalingKeys[i].mTime, vec3_from(in->mScalingKeys[i].mValue)});
//   return out;
// }

static void traverse_skeleton_nodes(scene& s,
                                    scene_node& node,
                                    uint32 parent,
                                    glm::mat4 space) {
  if (node.bone_entries.empty()) {
    for (auto& child : node.children)
      traverse_skeleton_nodes(s, child, parent, space * node.transform);
    return;
  }

  s.skeleton.parents.push_back(parent);
  s.skeleton.nodes.emplace_back(&node);
  parent = s.skeleton.bones.size();
  s.skeleton.bones.emplace_back(node.bone_entries[0].offset,
                                space * node.transform);

  for (auto& child : node.children)
    traverse_skeleton_nodes(s, child, parent, glm::mat4(1.0f));
}

static void update_skeleton(scene& s) {
  traverse_skeleton_nodes(s, s.root, -1, glm::mat4(1.0f));

  //
  s.skeleton.weights.resize(s.meshes.size());
  for (size_t mid = 0; auto& wdata : s.skeleton.weights) {
    wdata.offsets.assign(s.meshes[mid].vertices.size() + 1, 0);
    ++mid;
  }
  // Get the counts
  for (size_t bid = 0; bid < s.skeleton.bones.size(); ++bid) {
    auto& node = *s.skeleton.nodes[bid];
    for (auto& [mid, _, weights] : node.bone_entries) {
      for (auto& [vid, weight] : weights)
        ++s.skeleton.weights[mid].offsets[vid];
    }
  }
  // accumulate and allocate
  for (auto& data : s.skeleton.weights) {
    for (size_t i = 1; i < data.offsets.size(); ++i)
      data.offsets[i] += data.offsets[i - 1];
    data.data.resize(data.offsets.back());
  }
  // assign weights
  for (size_t bid = 0; bid < s.skeleton.bones.size(); ++bid) {
    auto& node = *s.skeleton.nodes[bid];
    for (auto& [mid, _, weights] : node.bone_entries) {
      for (auto& [vid, weight] : weights) {
        auto& wdata = s.skeleton.weights[mid];
        wdata.data[--wdata.offsets[vid]] = {bid, weight};
      }
    }
  }
}

auto scene_from_file(const std::filesystem::path& path) -> scene {
  // Generate functor for prefixed error messages.
  const auto throw_error = [&](czstring str) {
    throw std::runtime_error(std::format(
        "Failed to load scene from file '{}'. {}", path.string(), str));
  };

  // Check for file existence to receive more valuable error messages.
  if (!exists(path)) throw_error("The path does not exist.");

  Assimp::Importer importer{};

  // After the stripping and loading,
  // certain post processing steps are mandatory.
  const auto post_processing =
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent |
      /*aiProcess_OptimizeMeshes |*/ /*aiProcess_OptimizeGraph |*/
      aiProcess_FindDegenerates /*| aiProcess_DropNormals*/;

  const auto in = importer.ReadFile(path.c_str(), post_processing);

  // Check whether Assimp could load the file at all.
  if (!in || in->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !in->mRootNode)
    throw_error("Assimp could not process the file.");

  scene out{};

  // Name
  out.name = in->mName.C_Str();

  // Meshes
  out.meshes.reserve(in->mNumMeshes);
  for (size_type mid = 0; mid < in->mNumMeshes; ++mid)
    out.meshes.push_back(scene_mesh_from(in->mMeshes[mid]));

  // Hierarchy
  // out.hierarchy = scene_hierarchy_from(in->mRootNode);
  load_node(in->mRootNode, out.root);
  update_node_name_map(out);
  load_bone_entries(in, out);

  // Animations
  out.animations.reserve(in->mNumAnimations);
  for (size_type i = 0; i < in->mNumAnimations; ++i)
    out.animations.push_back(scene_animation_from(in->mAnimations[i]));

  update_skeleton(out);

  return out;
}

}  // namespace ensketch::sandbox
