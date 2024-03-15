#pragma once
#include <ensketch/opengl/shader_object.hpp>

namespace ensketch::opengl {

struct shader_program_ref : object_handle {
  using base = object_handle;
  using base::base;

  // using const_ref = shader_program_ref<false>;
  using this_ref = shader_program_ref;

  // constexpr auto ref() noexcept -> this_ref { return handle; }
  // constexpr auto ref() const noexcept -> const_ref { return handle; }

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

  explicit operator bool() const noexcept { return linked(); }

  auto info_log() const -> string {
    GLint info_log_size;
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &info_log_size);
    string info_log{};
    info_log.resize(info_log_size);
    glGetProgramInfoLog(handle, info_log_size, nullptr, info_log.data());
    return info_log;
  }

  void use() const noexcept { glUseProgram(handle); }

  void attach(shader_object_handle shader) noexcept {
    glAttachShader(id(), shader.id());
  }

  void detach(shader_object_handle shader) noexcept {
    glDetachShader(id(), shader.id());
  }

  // void attach(const_ref<vertex_shader> shader) noexcept {
  //   // assert(shader.compiled());
  //   // shader.compile();
  //   // const shader_object_handle tmp = shader;
  //   // tmp.compile();
  //   glAttachShader(id(), shader.id());
  // }

  // void attach(const_ref<fragment_shader> shader) noexcept {
  //   glAttachShader(id(), shader.id());
  // }

  // void detach(const_ref<vertex_shader> shader) noexcept {
  //   glDetachShader(id(), shader.id());
  // }

  // void detach(const_ref<fragment_shader> shader) noexcept {
  //   glDetachShader(id(), shader.id());
  // }

  // template <GLenum shader_object_type>
  // void attach(const_ref<shader_object<shader_object_type>> shader) noexcept {
  //   glAttachShader(id(), shader.id());
  // }
  // template <GLenum shader_object_type>
  // void detach(const_ref<shader_object<shader_object_type>> shader) noexcept {
  //   glDetachShader(id(), shader.id());
  // }

  void link() noexcept { glLinkProgram(handle); }

  void set(czstring name, auto&& value) {
    try_set(valid_uniform_location(name), forward<decltype(value)>(value));
  }

  void try_set(czstring name, auto&& value) noexcept {
    try_set(uniform_location(name), forward<decltype(value)>(value));
  }

 private:
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

  void try_set(GLint location, float value) noexcept {
    glProgramUniform1f(handle, location, value);
  }
  void try_set(GLint location, float x, float y) noexcept {
    glProgramUniform2f(handle, location, x, y);
  }
  void try_set(GLint location, float x, float y, float z) noexcept {
    glProgramUniform3f(handle, location, x, y, z);
  }
  void try_set(GLint location, float x, float y, float z, float w) noexcept {
    glProgramUniform4f(handle, location, x, y, z, w);
  }

  void try_set(GLint location, GLint value) noexcept {
    glProgramUniform1i(handle, location, value);
  }
  void try_set(GLint location, GLint x, GLint y) noexcept {
    glProgramUniform2i(handle, location, x, y);
  }
  void try_set(GLint location, GLint x, GLint y, GLint z) noexcept {
    glProgramUniform3i(handle, location, x, y, z);
  }
  void try_set(GLint location, GLint x, GLint y, GLint z, GLint w) noexcept {
    glProgramUniform4i(handle, location, x, y, z, w);
  }

  void try_set(GLint location, GLuint value) noexcept {
    glProgramUniform1ui(handle, location, value);
  }
  void try_set(GLint location, GLuint x, GLuint y) noexcept {
    glProgramUniform2ui(handle, location, x, y);
  }
  void try_set(GLint location, GLuint x, GLuint y, GLuint z) noexcept {
    glProgramUniform3ui(handle, location, x, y, z);
  }
  void try_set(GLint location,
               GLuint x,
               GLuint y,
               GLuint z,
               GLuint w) noexcept {
    glProgramUniform4ui(handle, location, x, y, z, w);
  }

  void try_set(GLint location, const vec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  void try_set(GLint location, const vec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  void try_set(GLint location, const vec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  void try_set(GLint location, const ivec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  void try_set(GLint location, const ivec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  void try_set(GLint location, const ivec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  void try_set(GLint location, const uvec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  void try_set(GLint location, const uvec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  void try_set(GLint location, const uvec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  void try_set(GLint location, const mat2& value) noexcept {
    glProgramUniformMatrix2fv(handle, location, 1, GL_FALSE, value_ptr(value));
  }
  void try_set(GLint location, const mat3& value) noexcept {
    glProgramUniformMatrix3fv(handle, location, 1, GL_FALSE, value_ptr(value));
  }
  void try_set(GLint location, const mat4& value) noexcept {
    glProgramUniformMatrix4fv(handle, location, 1, GL_FALSE, value_ptr(value));
  }

  void try_set(GLint location, const mat2x3& value) noexcept {
    glProgramUniformMatrix2x3fv(handle, location, 1, GL_FALSE,
                                value_ptr(value));
  }
  void try_set(GLint location, const mat3x2& value) noexcept {
    glProgramUniformMatrix3x2fv(handle, location, 1, GL_FALSE,
                                value_ptr(value));
  }

  void try_set(GLint location, const mat2x4& value) noexcept {
    glProgramUniformMatrix2x4fv(handle, location, 1, GL_FALSE,
                                value_ptr(value));
  }
  void try_set(GLint location, const mat4x2& value) noexcept {
    glProgramUniformMatrix4x2fv(handle, location, 1, GL_FALSE,
                                value_ptr(value));
  }

  void try_set(GLint location, const mat3x4& value) noexcept {
    glProgramUniformMatrix3x4fv(handle, location, 1, GL_FALSE,
                                value_ptr(value));
  }
  void try_set(GLint location, const mat4x3& value) noexcept {
    glProgramUniformMatrix4x3fv(handle, location, 1, GL_FALSE,
                                value_ptr(value));
  }
};

// using shader_program_const_ref = shader_program_ref<false>;
// using shader_program_ref = shader_program_ref<true>;

class shader_program final : public shader_program_ref {
 public:
  using base = shader_program_ref;

  // operator shader_object_ref() noexcept { return handle; }
  // operator shader_program_const_ref() const noexcept { return handle; }
  // auto ref() noexcept -> shader_program_ref { return handle; }
  // auto ref() const noexcept -> shader_program_const_ref { return handle; }

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

// static_assert(same_as<ref<shader_program_ref>, shader_program_ref>);
// static_assert(same_as<ref<shader_program_ref&>, shader_program_ref>);
// static_assert(same_as<ref<const shader_program_ref>, shader_program_const_ref>);
// static_assert(
//     same_as<ref<const shader_program_ref&>, shader_program_const_ref>);
// //
// static_assert(same_as<const_ref<shader_program_ref>, shader_program_const_ref>);
// static_assert(
//     same_as<const_ref<shader_program_ref&>, shader_program_const_ref>);
// static_assert(
//     same_as<const_ref<const shader_program_ref>, shader_program_const_ref>);
// static_assert(
//     same_as<const_ref<const shader_program_ref&>, shader_program_const_ref>);
// //
// static_assert(same_as<ref<shader_program_const_ref>, shader_program_const_ref>);
// static_assert(
//     same_as<ref<shader_program_const_ref&>, shader_program_const_ref>);
// static_assert(
//     same_as<ref<const shader_program_const_ref>, shader_program_const_ref>);
// static_assert(
//     same_as<ref<const shader_program_const_ref&>, shader_program_const_ref>);
// //
// static_assert(
//     same_as<const_ref<shader_program_const_ref>, shader_program_const_ref>);
// static_assert(
//     same_as<const_ref<shader_program_const_ref&>, shader_program_const_ref>);
// static_assert(same_as<const_ref<const shader_program_const_ref>,
//                       shader_program_const_ref>);
// static_assert(same_as<const_ref<const shader_program_const_ref&>,
//                       shader_program_const_ref>);
// //
// static_assert(same_as<ref<shader_program>, shader_program_ref>);
// static_assert(same_as<ref<shader_program&>, shader_program_ref>);
// static_assert(same_as<ref<const shader_program>, shader_program_const_ref>);
// static_assert(same_as<ref<const shader_program&>, shader_program_const_ref>);
// //
// static_assert(same_as<const_ref<shader_program>, shader_program_const_ref>);
// static_assert(same_as<const_ref<shader_program&>, shader_program_const_ref>);
// static_assert(
//     same_as<const_ref<const shader_program>, shader_program_const_ref>);
// static_assert(
//     same_as<const_ref<const shader_program&>, shader_program_const_ref>);

}  // namespace ensketch::opengl
