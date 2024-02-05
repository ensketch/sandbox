#pragma once
#include <ensketch/opengl/shader_object.hpp>
#include <ensketch/opengl/shader_program.hpp>

namespace ensketch::opengl {

inline auto shader_from_file(const filesystem::path& path) -> shader_program {
  if (!is_directory(path))
    throw runtime_error("Failed to load GLSL shader. Path '"s + path.string() +
                        "' is not a directory.");

  const auto vs_path = path / "vs.glsl";
  if (!is_regular_file(vs_path))
    throw runtime_error("Vertex shader file '" + vs_path.string() +
                        "' does not exist.");
  const auto vs = vertex_shader_from_file(vs_path.c_str());

  const auto fs_path = path / "fs.glsl";
  if (!is_regular_file(fs_path))
    throw runtime_error("Fragment shader file '" + fs_path.string() +
                        "' does not exist.");
  const auto fs = fragment_shader_from_file(fs_path.c_str());

  const auto gs_path = path / "gs.glsl";
  if (is_regular_file(gs_path)) {
    const auto gs = geometry_shader_from_file(gs_path.c_str());
    return shader_program{vs, gs, fs};
  }

  return shader_program{vs, fs};
}

}  // namespace ensketch::opengl
