#include <ensketch/sandbox/skeletal_mesh.hpp>
//
#include <ensketch/sandbox/log.hpp>
//
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

namespace ensketch::sandbox {

auto aabb_from(const skeletal_mesh& mesh) noexcept -> aabb3 {
  return ensketch::sandbox::aabb_from(
      mesh.vertices |
      views::transform([](const auto& x) { return x.position; }));
}

auto skeletal_mesh_from_file(const std::filesystem::path& path)
    -> skeletal_mesh {
  // Generate functor for prefixed error messages.
  //
  const auto throw_error = [&](czstring str) {
    throw std::runtime_error("Failed to load skeletal from path '"s +
                             path.string() + "'. " + str);
  };

  if (!exists(path)) throw_error("The path does not exist.");

  Assimp::Importer importer{};

  // After the stripping and loading,
  // certain post processing steps are mandatory.
  const auto post_processing =
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent |
      /*aiProcess_OptimizeMeshes |*/ /*aiProcess_OptimizeGraph |*/
      aiProcess_FindDegenerates /*| aiProcess_DropNormals*/;

  const auto scene = importer.ReadFile(path.c_str(), post_processing);

  // Check whether Assimp could load the file at all.
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    throw_error("Assimp could not process the file.");

  // if (scene->mNumMeshes > 1)
  //   throw_error("More than one mesh is not supported, yet.");

  auto mesh = scene->mMeshes[0];

  skeletal_mesh result{};

  // Vertices of the result
  result.vertices.resize(mesh->mNumVertices);
  for (size_t vid = 0; vid < mesh->mNumVertices; ++vid) {
    result.vertices[vid] = {.position = {mesh->mVertices[vid].x,  //
                                         mesh->mVertices[vid].y,  //
                                         mesh->mVertices[vid].z},
                            .normal = {mesh->mNormals[vid].x,  //
                                       mesh->mNormals[vid].y,  //
                                       mesh->mNormals[vid].z}};

    for (size_t i = 0; i < skeletal_mesh::max_bone_influence; ++i)
      result.vertices[vid].bone_ids[i] = skeletal_mesh::invalid;
  }

  // Bones
  result.bones.resize(mesh->mNumBones);
  for (size_t bid = 0; bid < mesh->mNumBones; ++bid) {
    log::info(std::format("bone name: {}", mesh->mBones[bid]->mName.C_Str()));

    auto bone = mesh->mBones[bid];

    auto mat4_from = [](const aiMatrix4x4& from) {
      glm::mat4 to;
      // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
      to[0][0] = from.a1;
      to[1][0] = from.a2;
      to[2][0] = from.a3;
      to[3][0] = from.a4;
      to[0][1] = from.b1;
      to[1][1] = from.b2;
      to[2][1] = from.b3;
      to[3][1] = from.b4;
      to[0][2] = from.c1;
      to[1][2] = from.c2;
      to[2][2] = from.c3;
      to[3][2] = from.c4;
      to[0][3] = from.d1;
      to[1][3] = from.d2;
      to[2][3] = from.d3;
      to[3][3] = from.d4;
      return to;
    };

    result.bones[bid].offset = mat4_from(bone->mOffsetMatrix);

    for (size_t wid = 0; wid < bone->mNumWeights; ++wid) {
      auto vid = bone->mWeights[wid].mVertexId;
      float weight = bone->mWeights[wid].mWeight;

      for (size_t i = 0; i < skeletal_mesh::max_bone_influence; ++i) {
        if (result.vertices[vid].bone_ids[i] != skeletal_mesh::invalid)
          continue;
        result.vertices[vid].bone_ids[i] = bid;
        result.vertices[vid].bone_weights[i] = weight;
      }
    }
  }

  // Faces of the result
  result.faces.resize(mesh->mNumFaces);
  for (size_t fid = 0; fid < mesh->mNumFaces; ++fid) {
    auto face = mesh->mFaces[fid];
    // All faces need to be triangles.
    // So, use a simple triangulation of polygons.
    const auto corners = face.mNumIndices;
    for (size_t k = 2; k < corners; ++k) {
      result.faces[fid] = {face.mIndices[0],      //
                           face.mIndices[k - 1],  //
                           face.mIndices[k]};
    }
  }

  return result;
}

}  // namespace ensketch::sandbox
