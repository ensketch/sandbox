#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

constexpr auto shader_object_type_name(GLenum shader_object_type) -> czstring {
  switch (shader_object_type) {
    case GL_VERTEX_SHADER:
      return "vertex";
      break;

    case GL_GEOMETRY_SHADER:
      return "geometry";
      break;

    case GL_FRAGMENT_SHADER:
      return "fragment";
      break;

    default:
      return "unknown";
  }
}

inline constexpr struct warnings_as_errors_t {
} warnings_as_errors{};
inline constexpr struct ignore_warnings_t {
} ignore_warnings{};

struct shader_compile_error : runtime_error {
  using base = runtime_error;
  shader_compile_error(auto&& x) : base(forward<decltype(x)>(x)) {}
};

template <GLenum shader_object_type>
class shader_object {
  using string = std::string;

 public:
  // struct shader_compile_error : runtime_error {
  //   using base = runtime_error;
  //   shader_compile_error(auto&& x) : base(forward<decltype(x)>(x)) {}
  // };

  static constexpr auto type() -> GLenum { return shader_object_type; }
  static constexpr auto type_name() -> czstring {
    return shader_object_type_name(type());
  }

  shader_object() = default;

  // Using exceptions for warnings is a bad idea,
  // because they break execution order.

  // Create the shader object by compiling the source code
  // and then writing its info log into the given string reference.

  shader_object(czstring source, auto&& warning_handle) {
    receive_handle();
    compile(source, std::forward<decltype(warning_handle)>(warning_handle));
  }

  shader_object(czstring source) : shader_object{source, warnings_as_errors} {}

  ~shader_object() {
    // Zero values are ignored by this function.
    glDeleteShader(handle);
  }

  // Copying is not allowed.
  shader_object(const shader_object&) = delete;
  shader_object& operator=(const shader_object&) = delete;

  // Moving
  shader_object(shader_object&& x) : handle{x.handle} { x.handle = 0; }
  shader_object& operator=(shader_object&& x) {
    swap(handle, x.handle);
    return *this;
  }

  // This class is only a wrapper.
  // Because of that, the underlying handle can also
  // be used directly and implicitly.
  // This may introduce consistency problems.
  operator GLuint() const { return handle; }

  // Checks if shader object handles an actual shader.
  // Should return false when called after the default constructor.
  bool exists() noexcept {
    // May already be enough when handle is not used from the outside.
    // return handle;
    // But this approach is more secure.
    return glIsShader(handle) == GL_TRUE;
  }

 private:
  void receive_handle() {
    handle = glCreateShader(shader_object_type);
    if (!handle)
      throw runtime_error(string("Failed to receive handle for ") +
                          type_name() + " shader object.");
  }

  bool compile_failed() noexcept {
    GLint compile_status;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &compile_status);
    return static_cast<GLboolean>(compile_status) == GL_FALSE;
  }

  void write_compile_info_log(string& info_log) {
    GLint info_log_size;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &info_log_size);
    info_log.resize(info_log_size);
    if (info_log_size)
      glGetShaderInfoLog(handle, info_log_size, nullptr, info_log.data());
  }

  void throw_compile_error() {
    throw shader_compile_error(string("Failed to compile ") + type_name() +
                               " shader object.");
  }

  void throw_compile_error(const string& info_log) {
    throw shader_compile_error(string("Failed to compile ") + type_name() +
                               " shader object.\n" + info_log);
  }

  void compile(czstring source) {
    glShaderSource(handle, 1, &source, nullptr);
    glCompileShader(handle);
  }

  void compile(czstring source, string& info_log) {
    compile(source);
    write_compile_info_log(info_log);
    if (compile_failed()) throw_compile_error(info_log);
  }

  void compile(czstring source, warnings_as_errors_t) {
    compile(source);
    string info_log;
    write_compile_info_log(info_log);
    if (compile_failed() || !info_log.empty()) throw_compile_error(info_log);
  }

  void compile(czstring source, ignore_warnings_t) {
    compile(source);
    if (compile_failed()) {
      string info_log;
      write_compile_info_log(info_log);
      throw_compile_error(info_log);
    }
  }

  void compile(czstring source, auto&& warning_callback) {
    string info_log;
    compile(source, info_log);
    if (!info_log.empty())
      std::forward<decltype(warning_callback)>(warning_callback)(info_log);
  }

  GLuint handle{};
};

using vertex_shader = shader_object<GL_VERTEX_SHADER>;
using geometry_shader = shader_object<GL_GEOMETRY_SHADER>;
using fragment_shader = shader_object<GL_FRAGMENT_SHADER>;

inline auto vertex_shader_from_file(czstring path, auto&& warning_handle)
    -> vertex_shader {
  return vertex_shader{string_from_file(path).c_str(),
                       std::forward<decltype(warning_handle)>(warning_handle)};
}

inline auto vertex_shader_from_file(czstring path) {
  return vertex_shader_from_file(path, warnings_as_errors);
}

inline auto geometry_shader_from_file(
    czstring path,
    auto&& warning_handle = warnings_as_errors) -> geometry_shader {
  return geometry_shader{
      string_from_file(path).c_str(),
      std::forward<decltype(warning_handle)>(warning_handle)};
}

inline auto geometry_shader_from_file(czstring path) {
  return geometry_shader_from_file(path, warnings_as_errors);
}

inline auto fragment_shader_from_file(
    czstring path,
    auto&& warning_handle = warnings_as_errors) -> fragment_shader {
  return fragment_shader{
      string_from_file(path).c_str(),
      std::forward<decltype(warning_handle)>(warning_handle)};
}

inline auto fragment_shader_from_file(czstring path) {
  return fragment_shader_from_file(path, warnings_as_errors);
}

}  // namespace ensketch::opengl
