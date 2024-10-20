#include <ensketch/sandbox/flat_scene.hpp>
//
#include <ensketch/sandbox/log.hpp>
//
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

namespace ensketch::sandbox {

using size_type = flat_scene::size_type;

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

auto aabb_from(const flat_scene& scene) noexcept -> aabb3 {
  return sandbox::aabb_from(
      scene.vertices |
      views::transform([](const auto& x) { return x.position; }));
}

static void allocate_mesh_data(const aiScene* in, flat_scene& out) {
  size_type vertex_count = 0;
  size_type face_count = 0;
  for (size_type mid = 0; mid < in->mNumMeshes; ++mid) {
    vertex_count += in->mMeshes[mid]->mNumVertices;
    face_count += in->mMeshes[mid]->mNumFaces;
  }
  out.vertices.reserve(vertex_count);
  out.faces.reserve(face_count);
  out.meshes.reserve(in->mNumMeshes);
}

static void load_mesh_data(const aiScene* in, flat_scene& out) {
  allocate_mesh_data(in, out);

  // Load the actual vertices and faces data.
  size_type voffset = 0;
  size_type foffset = 0;
  for (size_type mid = 0; mid < in->mNumMeshes; ++mid) {
    const auto mesh = in->mMeshes[mid];

    // Vertices
    for (size_type vid = 0; vid < mesh->mNumVertices; ++vid)
      out.vertices.push_back(
          flat_scene::vertex{.position = vec3_from(mesh->mVertices[vid]),
                             .normal = vec3_from(mesh->mNormals[vid])});

    // Faces
    for (size_type fid = 0; fid < mesh->mNumFaces; ++fid) {
      auto face = mesh->mFaces[fid];
      // All faces need to be triangles.
      // So, use a simple triangulation of polygons.
      for (size_type k = 2; k < face.mNumIndices; ++k) {
        out.faces.push_back({voffset + face.mIndices[0],      //
                             voffset + face.mIndices[k - 1],  //
                             voffset + face.mIndices[k]});
      }
    }

    // Mesh
    out.meshes.emplace_back(
        mesh->mName.C_Str(),
        flat_scene::index_span{voffset,
                               static_cast<size_type>(out.vertices.size())},
        flat_scene::index_span{foffset,
                               static_cast<size_type>(out.faces.size())});

    voffset = out.vertices.size();
    foffset = out.faces.size();
  }
}

static auto traverse_and_assign_nodes(const aiNode* in,
                                      struct flat_scene::hierarchy& out,
                                      size_type parent = 0) -> size_type {
  out.nodes.emplace_back(
      in->mName.C_Str(), mat4_from(in->mTransformation), parent,
      flat_scene::index_span{
          static_cast<size_type>(out.children.size()),
          static_cast<size_type>(out.children.size() + in->mNumChildren)});

  // `out.children.size()` determines the state.
  // It needs to be updated before recursively processing the children.
  // Children Indices
  const size_type current = out.nodes.size() - 1;
  const auto offset = out.children.size();
  out.children.resize(out.children.size() + in->mNumChildren);
  for (size_type i = 0; i < in->mNumChildren; ++i)
    out.children[offset + i] =
        traverse_and_assign_nodes(in->mChildren[i], out, current);
  return current;
}

static void load_hierarchy(const aiScene* in, flat_scene& out) {
  traverse_and_assign_nodes(in->mRootNode, out.hierarchy);
}

auto flat_scene_from_file(const std::filesystem::path& path) -> flat_scene {
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

  flat_scene out{};
  out.name = in->mName.C_Str();
  load_mesh_data(in, out);
  load_hierarchy(in, out);
  return out;
}

}  // namespace ensketch::sandbox
