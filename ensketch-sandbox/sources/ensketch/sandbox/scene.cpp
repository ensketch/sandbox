#include <ensketch/sandbox/scene.hpp>
//
#include <ensketch/sandbox/log.hpp>
//
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

namespace ensketch::sandbox {

static auto vec3_from(const aiVector3D& v) noexcept -> glm::vec3 {
  return {v.x, v.y, v.z};
};

static auto quat_from(const aiQuaternion& q) noexcept -> glm::quat {
  return {q.w, q.x, q.y, q.z};
}

static auto mat4_from(const aiMatrix4x4& from) noexcept -> glm::mat4 {
  return {from.a1, from.b1, from.c1, from.d1,  //
          from.a2, from.b2, from.c2, from.d2,  //
          from.a3, from.b3, from.c3, from.d3,  //
          from.a4, from.b4, from.c4, from.d4};
};

static void load(const aiMesh* in, scene::mesh& out) {
  // Name
  out.name = in->mName.C_Str();
  // Vertices
  out.vertices.reserve(in->mNumVertices);
  for (size_t vid = 0; vid < in->mNumVertices; ++vid)
    out.vertices.emplace_back(vec3_from(in->mVertices[vid]),
                              vec3_from(in->mNormals[vid]));
  // Faces
  out.faces.reserve(in->mNumFaces);
  for (size_t fid = 0; fid < in->mNumFaces; ++fid) {
    auto face = in->mFaces[fid];
    // All faces need to be triangles.
    // So, use a simple triangulation of polygons.
    for (size_t k = 2; k < face.mNumIndices; ++k)
      out.faces.push_back({face.mIndices[0],      //
                           face.mIndices[k - 1],  //
                           face.mIndices[k]});
  }
  // Bones
  // The `scene` data structure stores all bone information and weights
  // in the its hierarchy's nodes and therefore bones are not handled here.
}

static void load_meshes(const aiScene* in, scene& out) {
  out.meshes.resize(in->mNumMeshes);
  for (size_t mid = 0; mid < in->mNumMeshes; ++mid)
    load(in->mMeshes[mid], out.meshes[mid]);
}

static void load(const aiNode* in,
                 scene::node& out,
                 uint32& count,
                 scene::node* parent = nullptr) {
  // Index
  out.index = count++;
  // Name
  out.name = in->mName.C_Str();
  // Matrices
  out.transform = mat4_from(in->mTransformation);
  // Contained Meshes
  out.meshes.reserve(in->mNumMeshes);
  for (size_t i = 0; i < in->mNumMeshes; ++i)
    out.meshes.push_back(in->mMeshes[i]);
  // Connectivity
  out.parent = parent;
  for (size_t i = 0; i < in->mNumChildren; ++i)
    load(in->mChildren[i], out.children.emplace_back(), count, &out);
}

static void update_node_name_map(scene& s) {
  s.node_name_map.clear();
  traverse(s.root, [&s](scene::node& node) {
    s.node_name_map.emplace(node.name, node);
  });
}

static void load_bone_entries(const aiScene* in, scene& out) {
  for (size_t mid = 0; mid < in->mNumMeshes; ++mid) {
    auto mesh = in->mMeshes[mid];
    for (size_t bid = 0; bid < mesh->mNumBones; ++bid) {
      auto bone = mesh->mBones[bid];
      auto& node = out.node_name_map.at(bone->mName.C_Str());
      node.offset = mat4_from(bone->mOffsetMatrix);
      auto& entry = node.bone_entries.emplace_back(mid);
      entry.weights.reserve(bone->mNumWeights);
      for (size_t wid = 0; wid < bone->mNumWeights; ++wid) {
        const auto vid = bone->mWeights[wid].mVertexId;
        const float weight = bone->mWeights[wid].mWeight;
        entry.weights.emplace_back(vid, weight);
      }
    }
  }
}

static void load_hierarchy(const aiScene* in, scene& out) {
  load(in->mRootNode, out.root, out.node_count);
  update_node_name_map(out);
  load_bone_entries(in, out);
}

auto scene::animation::channel::position(float64 time) const -> glm::mat4 {
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

auto scene::animation::channel::rotation(float64 time) const -> glm::mat4 {
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

auto scene::animation::channel::scaling(float64 time) const -> glm::mat4 {
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

auto scene::animation::channel::transform(float64 time) const -> glm::mat4 {
  return position(time) * rotation(time) * scaling(time);
}

static void load(const aiNodeAnim* in, scene::animation::channel& out) {
  // Node Name
  out.node_name = in->mNodeName.C_Str();
  // Position Keys
  out.positions.reserve(in->mNumPositionKeys);
  for (size_t i = 0; i < in->mNumPositionKeys; ++i)
    out.positions.push_back(
        {in->mPositionKeys[i].mTime, vec3_from(in->mPositionKeys[i].mValue)});
  // Rotation Keys
  out.rotations.reserve(in->mNumRotationKeys);
  for (size_t i = 0; i < in->mNumRotationKeys; ++i)
    out.rotations.push_back(
        {in->mRotationKeys[i].mTime, quat_from(in->mRotationKeys[i].mValue)});
  // Scaling Keys
  out.scalings.reserve(in->mNumScalingKeys);
  for (size_t i = 0; i < in->mNumScalingKeys; ++i)
    out.scalings.push_back(
        {in->mScalingKeys[i].mTime, vec3_from(in->mScalingKeys[i].mValue)});
}

static void load(const aiAnimation* in, scene::animation& out) {
  // Name
  out.name = in->mName.C_Str();
  // Duration
  out.duration = in->mDuration;
  out.ticks = in->mTicksPerSecond;
  // Bone Animation Channels
  out.channels.resize(in->mNumChannels);
  for (size_t i = 0; i < in->mNumChannels; ++i)
    load(in->mChannels[i], out.channels[i]);
}

static void load_animations(const aiScene* in, scene& out) {
  out.animations.resize(in->mNumAnimations);
  for (size_t i = 0; i < in->mNumAnimations; ++i)
    load(in->mAnimations[i], out.animations[i]);
}

static void traverse_skeleton_nodes(scene& s,
                                    scene::node& node,
                                    uint32 parent) {
  s.skeleton.parents.push_back(parent);
  s.skeleton.nodes.emplace_back(&node);
  parent = s.skeleton.bones.size();
  s.skeleton.bone_name_map.emplace(node.name, parent);
  s.skeleton.bones.emplace_back(node.offset, node.transform);

  for (auto& child : node.children) traverse_skeleton_nodes(s, child, parent);
}

static void update_skeleton(scene& s) {
  // traverse_skeleton_nodes(s, s.root, -1, glm::mat4(1.0f));
  traverse_skeleton_nodes(s, s.root, -1);

  //
  s.skeleton.weights.resize(s.meshes.size());
  for (size_t mid = 0; auto& wdata : s.skeleton.weights) {
    wdata.offsets.assign(s.meshes[mid].vertices.size() + 1, 0);
    ++mid;
  }
  // Get the counts
  for (size_t bid = 0; bid < s.skeleton.bones.size(); ++bid) {
    auto& node = *s.skeleton.nodes[bid];
    for (auto& [mid, weights] : node.bone_entries) {
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
    for (auto& [mid, weights] : node.bone_entries) {
      for (auto& [vid, weight] : weights) {
        auto& wdata = s.skeleton.weights[mid];
        wdata.data[--wdata.offsets[vid]] = {bid, weight};
      }
    }
  }
}

static void load(const aiScene* in, scene& out) {
  out.name = in->mName.C_Str();
  load_meshes(in, out);
  load_hierarchy(in, out);
  load_animations(in, out);
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

  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

  const auto in = importer.ReadFile(path.c_str(), post_processing);

  // Check whether Assimp could load the file at all.
  if (!in || in->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !in->mRootNode)
    throw_error("Assimp could not process the file.");

  scene out{};
  load(in, out);
  update_skeleton(out);
  return out;
}

auto aabb_from(const scene& s) noexcept -> aabb3 {
  aabb3 result{};
  for (const auto& mesh : s.meshes)
    for (const auto& v : mesh.vertices)
      result = sandbox::aabb_from(result, v.position);
  return result;
}

}  // namespace ensketch::sandbox
