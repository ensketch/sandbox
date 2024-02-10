#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

struct shader_object_ref : object_handle {
  using base = object_handle;
  using base::base;

  bool valid() const noexcept { return glIsShader(handle) == GL_TRUE; }

  bool compiled() const noexcept {
    // 'glGetShaderiv' will generate errors 'GL_INVALID_VALUE' or
    // 'GL_INVALID_OPERATION' if the shader handle is not valid.
    // If an error is generated, no change is made to 'status'.
    // Also, newly generated shader objects are not compiled.
    // So, 'status' can only be 'GL_TRUE'
    // if the shader is valid and compiled.
    //
    auto status = static_cast<GLint>(GL_FALSE);
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    return static_cast<GLboolean>(status) == GL_TRUE;
  }

  operator bool() const noexcept { return compiled(); }

  auto info_log() const -> string {
    GLint info_log_size;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &info_log_size);
    string info_log{};
    info_log.resize(info_log_size);
    glGetShaderInfoLog(handle, info_log_size, nullptr, info_log.data());
    return info_log;
  }

  using GLstring = const GLchar*;
  static auto data(czstring str) noexcept -> GLstring { return str; }
  static auto size(czstring str) noexcept -> GLint { return -1; }
  static auto data(string_view str) noexcept -> GLstring { return str.data(); }
  static auto size(string_view str) noexcept -> GLint { return str.size(); }

  void set_source(auto&&... str) noexcept {
    // OpenGL copies the shader source code strings
    // when glShaderSource is called, so an application
    // may free its copy of the source code strings
    // immediately after the function returns.
    //
    constexpr GLsizei count = sizeof...(str);
    array<GLstring, count> strings{data(str)...};
    array<GLint, count> lengths{size(str)...};
    glShaderSource(handle, count, strings.data(), lengths.data());
  }

  // Specialization for 'czstring'
  //
  // void set_source(czstring... str) noexcept {
  //   constexpr GLsizei count = sizeof...(str);
  //   array<GLstring, count> strings{str...};
  //   glShaderSource(handle, count, strings.data(), nullptr);
  // }

  void compile() noexcept { glCompileShader(handle); }

  bool compile(auto&&... str) {
    set_source(forward<decltype(str)>(str)...);
    compile();
    return compiled();
  }
};

constexpr auto shader_object_type_name(GLenum shader_object_type) -> czstring {
  switch (shader_object_type) {
    case GL_VERTEX_SHADER:
      return "vertex";
      break;

    case GL_TESS_CONTROL_SHADER:
      return "tessellation control";
      break;

    case GL_TESS_EVALUATION_SHADER:
      return "tessellation evaluation";
      break;

    case GL_GEOMETRY_SHADER:
      return "geometry";
      break;

    case GL_FRAGMENT_SHADER:
      return "fragment";
      break;

    case GL_COMPUTE_SHADER:
      return "compute";
      break;

    default:
      return "unknown";
  }
}

template <GLenum shader_object_type>
class shader_object final : public shader_object_ref {
 public:
  using base = shader_object_ref;

  static constexpr auto type() noexcept { return shader_object_type; }
  static constexpr auto type_name() noexcept {
    return shader_object_type_name(type());
  }

  shader_object() {
    handle = glCreateShader(shader_object_type);
    // Not receiving a valid handle is exceptional.
    if (!handle)
      throw runtime_error(format(
          "Failed to receive handle for {} shader object.", type_name()));
  }

  shader_object(auto&&... str) : shader_object{} {
    set_source(forward<decltype(str)>(str)...);
    compile();
  }

  ~shader_object() noexcept { glDeleteShader(handle); }

  // Copying is NOT allowed.
  //
  shader_object(const shader_object&) = delete;
  shader_object& operator=(const shader_object&) = delete;

  // Moving is allowed.
  //
  shader_object(shader_object&& x) : base{x.handle} { x.handle = 0; }
  shader_object& operator=(shader_object&& x) {
    swap(handle, x.handle);
    return *this;
  }
};

using vertex_shader = shader_object<GL_VERTEX_SHADER>;
using tessellation_control_shader = shader_object<GL_TESS_CONTROL_SHADER>;
using tessellation_evaluation_shader = shader_object<GL_TESS_EVALUATION_SHADER>;
using geometry_shader = shader_object<GL_GEOMETRY_SHADER>;
using fragment_shader = shader_object<GL_FRAGMENT_SHADER>;
using compute_shader = shader_object<GL_COMPUTE_SHADER>;

inline auto vertex_shader_from_file(const filesystem::path& path)
    -> vertex_shader {
  return vertex_shader{string_from_file(path)};
}

inline auto tessellation_control_shader_from_file(const filesystem::path& path)
    -> tessellation_control_shader {
  return tessellation_control_shader{string_from_file(path)};
}

inline auto tessellation_evaluation_shader_from_file(
    const filesystem::path& path) -> tessellation_evaluation_shader {
  return tessellation_evaluation_shader{string_from_file(path)};
}

inline auto geometry_shader_from_file(const filesystem::path& path)
    -> geometry_shader {
  return geometry_shader{string_from_file(path)};
}

inline auto fragment_shader_from_file(const filesystem::path& path)
    -> fragment_shader {
  return fragment_shader{string_from_file(path)};
}

inline auto compute_shader_from_file(const filesystem::path& path)
    -> compute_shader {
  return compute_shader{string_from_file(path)};
}

}  // namespace ensketch::opengl
