#pragma once
#include <ensketch/opengl/shader_object.hpp>

namespace ensketch::opengl {

struct shader_program_handle : object_handle {
  using base = object_handle;
  using base::base;

  bool valid() const noexcept { return glIsProgram(handle) == GL_TRUE; }

  bool linked() const noexcept {
    // 'glGetProgramiv' will generate errors 'GL_INVALID_VALUE' or
    // 'GL_INVALID_OPERATION' if the program handle is not valid.
    // If an error is generated, no change is made to 'status'.
    // Also, newly generated programs are not linked.
    // So, 'status' can only be 'GL_TRUE'
    // if the program is valid and linked.
    //
    auto status = static_cast<GLint>(GL_FALSE);
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    return static_cast<GLboolean>(status) == GL_TRUE;
  }

  operator bool() const noexcept { return linked(); }

  auto info_log() const -> string {
    GLint info_log_size;
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &info_log_size);
    string info_log{};
    info_log.resize(info_log_size);
    glGetProgramInfoLog(handle, info_log_size, nullptr, info_log.data());
    return info_log;
  }

  auto bind() const noexcept -> shader_program_handle {
    glUseProgram(handle);
    return handle;
  }

  void use() const noexcept { glUseProgram(handle); }

  void attach(shader_object_handle shader) noexcept {
    glAttachShader(handle, shader);
  }

  void detach(shader_object_handle shader) noexcept {
    glDetachShader(handle, shader);
  }

  void link() noexcept { glLinkProgram(handle); }

  auto set(czstring name, auto&& value) -> shader_program_handle {
    try_set(valid_uniform_location(name), forward<decltype(value)>(value));
    return *this;
  }

  auto try_set(czstring name, auto&& value) noexcept -> shader_program_handle {
    try_set(uniform_location(name), forward<decltype(value)>(value));
    return *this;
  }

  auto uniform_location(czstring name) noexcept {
    return glGetUniformLocation(handle, name);
  }
  auto valid_uniform_location(czstring name) {
    const auto result = uniform_location(name);
    if (result == -1)
      throw invalid_argument(
          string("Failed to receive valid uniform location for '") + name +
          "'.");
    return result;
  }

  static void try_set(GLint location, float value) noexcept {
    glUniform1f(location, value);
  }
  static void try_set(GLint location, float x, float y) noexcept {
    glUniform2f(location, x, y);
  }
  static void try_set(GLint location, float x, float y, float z) noexcept {
    glUniform3f(location, x, y, z);
  }
  static void try_set(GLint location,
                      float x,
                      float y,
                      float z,
                      float w) noexcept {
    glUniform4f(location, x, y, z, w);
  }

  static void try_set(GLint location, GLint value) noexcept {
    glUniform1i(location, value);
  }
  static void try_set(GLint location, GLint x, GLint y) noexcept {
    glUniform2i(location, x, y);
  }
  static void try_set(GLint location, GLint x, GLint y, GLint z) noexcept {
    glUniform3i(location, x, y, z);
  }
  static void try_set(GLint location,
                      GLint x,
                      GLint y,
                      GLint z,
                      GLint w) noexcept {
    glUniform4i(location, x, y, z, w);
  }

  static void try_set(GLint location, GLuint value) noexcept {
    glUniform1ui(location, value);
  }
  static void try_set(GLint location, GLuint x, GLuint y) noexcept {
    glUniform2ui(location, x, y);
  }
  static void try_set(GLint location, GLuint x, GLuint y, GLuint z) noexcept {
    glUniform3ui(location, x, y, z);
  }
  static void try_set(GLint location,
                      GLuint x,
                      GLuint y,
                      GLuint z,
                      GLuint w) noexcept {
    glUniform4ui(location, x, y, z, w);
  }

  static void try_set(GLint location, const vec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  static void try_set(GLint location, const vec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  static void try_set(GLint location, const vec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  static void try_set(GLint location, const ivec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  static void try_set(GLint location, const ivec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  static void try_set(GLint location, const ivec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  static void try_set(GLint location, const uvec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  static void try_set(GLint location, const uvec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  static void try_set(GLint location, const uvec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  static void try_set(GLint location, const mat2& value) noexcept {
    glUniformMatrix2fv(location, 1, GL_FALSE, value_ptr(value));
  }
  static void try_set(GLint location, const mat3& value) noexcept {
    glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(value));
  }
  static void try_set(GLint location, const mat4& value) noexcept {
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
  }

  static void try_set(GLint location, const mat2x3& value) noexcept {
    glUniformMatrix2x3fv(location, 1, GL_FALSE, value_ptr(value));
  }
  static void try_set(GLint location, const mat3x2& value) noexcept {
    glUniformMatrix3x2fv(location, 1, GL_FALSE, value_ptr(value));
  }

  static void try_set(GLint location, const mat2x4& value) noexcept {
    glUniformMatrix2x4fv(location, 1, GL_FALSE, value_ptr(value));
  }
  static void try_set(GLint location, const mat4x2& value) noexcept {
    glUniformMatrix4x2fv(location, 1, GL_FALSE, value_ptr(value));
  }

  static void try_set(GLint location, const mat3x4& value) noexcept {
    glUniformMatrix3x4fv(location, 1, GL_FALSE, value_ptr(value));
  }
  static void try_set(GLint location, const mat4x3& value) noexcept {
    glUniformMatrix4x3fv(location, 1, GL_FALSE, value_ptr(value));
  }
};

class shader_program final : public shader_program_handle {
 public:
  using base = shader_program_handle;

  shader_program() {
    handle = glCreateProgram();
    if (!handle)
      throw runtime_error(string("Failed to receive shader program handle."));
  }

  ~shader_program() noexcept { glDeleteProgram(handle); }

  // Copying is NOT allowed.
  //
  shader_program(const shader_program&) = delete;
  shader_program& operator=(const shader_program&) = delete;

  // Moving is allowed.
  //
  shader_program(shader_program&& x) : base{x.handle} { x.handle = 0; }
  shader_program& operator=(shader_program&& x) {
    swap(handle, x.handle);
    return *this;
  }
};

}  // namespace ensketch::opengl
