#include <ensketch/sandbox/polyhedral_surface.hpp>
//
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

namespace ensketch::sandbox {

auto polyhedral_surface_from(const stl_surface& data) -> polyhedral_surface {
  using size_type = polyhedral_surface::size_type;
  static_assert(same_as<size_type, stl_surface::size_type>);

  polyhedral_surface surface{};
  surface.vertices.resize(data.triangles.size() * 3);
  surface.faces.resize(data.triangles.size());

  for (size_type i = 0; i < data.triangles.size(); ++i) {
    for (size_type j = 0; j < 3; ++j)
      surface.vertices[3 * i + j] = {
          .position = data.triangles[i].vertex[j],
          .normal = data.triangles[i].normal,
      };
    surface.faces[i] = {3 * i + 0, 3 * i + 1, 3 * i + 2};
  }

  return surface;
}

auto polyhedral_surface_from(const filesystem::path& path)
    -> polyhedral_surface {
  // Generate functor for prefixed error messages.
  //
  const auto throw_error = [&](czstring str) {
    throw runtime_error("Failed to load 'polyhedral_surface' from path '"s +
                        path.string() + "'. " + str);
  };

  if (!exists(path)) throw_error("The path does not exist.");

  // Use a custom loader for STL files.
  //
  if (path.extension().string() == ".stl" ||
      path.extension().string() == ".STL")
    return polyhedral_surface_from(stl_surface(path));

  // For all other file formats, assimp will do the trick.
  //
  Assimp::Importer importer{};

  // Assimp only needs to generate a continuously connected surface.
  // So, a lot of information can be stripped from vertices.
  //
  importer.SetPropertyInteger(
      AI_CONFIG_PP_RVC_FLAGS,
      /*aiComponent_NORMALS |*/ aiComponent_TANGENTS_AND_BITANGENTS |
          aiComponent_COLORS |
          /*aiComponent_TEXCOORDS |*/ aiComponent_BONEWEIGHTS |
          aiComponent_ANIMATIONS | aiComponent_TEXTURES | aiComponent_LIGHTS |
          aiComponent_CAMERAS /*| aiComponent_MESHES*/ | aiComponent_MATERIALS);

  // After the stripping and loading,
  // certain post processing steps are mandatory.
  //
  const auto post_processing =
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent |
      /*aiProcess_OptimizeMeshes |*/ /*aiProcess_OptimizeGraph |*/
      aiProcess_FindDegenerates /*| aiProcess_DropNormals*/;

  // Now, let Assimp actually load a surface scene from the given file.
  //
  const auto scene = importer.ReadFile(path.c_str(), post_processing);

  // Check whether Assimp could load the file at all.
  //
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    throw_error("Assimp could not process the file.");

  // Now, transform the loaded mesh data from
  // Assimp's internal structure to a polyhedral surface.
  //
  polyhedral_surface surface{};

  // First, get the total number of vertices
  // and faces and for all meshes.
  //
  size_t vertex_count = 0;
  size_t face_count = 0;
  for (size_t i = 0; i < scene->mNumMeshes; ++i) {
    vertex_count += scene->mMeshes[i]->mNumVertices;
    face_count += scene->mMeshes[i]->mNumFaces;
  }
  //
  surface.vertices.resize(vertex_count);
  surface.faces.resize(face_count);

  // Iterate over all meshes and get the vertex and face information.
  // All meshes will be linearly stored in one polyhedral surface.
  //
  uint32 vertex_offset = 0;
  uint32 face_offset = 0;
  for (size_t mid = 0; mid < scene->mNumMeshes; ++mid) {
    // Vertices of the Mesh
    //
    for (size_t vid = 0; vid < scene->mMeshes[mid]->mNumVertices; ++vid) {
      surface.vertices[vid + vertex_offset] = {
          .position = {scene->mMeshes[mid]->mVertices[vid].x,  //
                       scene->mMeshes[mid]->mVertices[vid].y,  //
                       scene->mMeshes[mid]->mVertices[vid].z},
          .normal = {scene->mMeshes[mid]->mNormals[vid].x,  //
                     scene->mMeshes[mid]->mNormals[vid].y,  //
                     scene->mMeshes[mid]->mNormals[vid].z}};
    }

    // Faces of the Mesh
    //
    for (size_t fid = 0; fid < scene->mMeshes[mid]->mNumFaces; ++fid) {
      // All faces need to be triangles.
      // So, use a simple triangulation of polygons.
      const auto corners = scene->mMeshes[mid]->mFaces[fid].mNumIndices;
      for (size_t k = 2; k < corners; ++k) {
        surface.faces[face_offset + fid] = {
            scene->mMeshes[mid]->mFaces[fid].mIndices[0] + vertex_offset,  //
            scene->mMeshes[mid]->mFaces[fid].mIndices[k - 1] +
                vertex_offset,  //
            scene->mMeshes[mid]->mFaces[fid].mIndices[k] + vertex_offset};
      }
    }

    // Update offsets to not overwrite previously written meshes.
    //
    vertex_offset += scene->mMeshes[mid]->mNumVertices;
    face_offset += scene->mMeshes[mid]->mNumFaces;
  }

  return surface;
}

auto aabb_from(const polyhedral_surface& surface) noexcept -> aabb3 {
  return ensketch::sandbox::aabb_from(
      surface.vertices |
      views::transform([](const auto& x) { return x.position; }));
}

}  // namespace ensketch::sandbox
