#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

struct shader_link_error : runtime_error {
  using base = runtime_error;
  shader_link_error(auto&& x) : base(std::forward<decltype(x)>(x)) {}
};

struct shader_program_handle : object_handle {
  using base = object_handle;
  using base::base;

  auto bind() const noexcept -> shader_program_handle {
    glUseProgram(handle);
    return handle;
  }

  bool exists() const { return glIsProgram(handle) == GL_TRUE; }

  auto set(czstring name, auto&& value) -> shader_program_handle {
    try_set(valid_uniform_location(name), forward<decltype(value)>(value));
    return *this;
  }

  auto try_set(czstring name, auto&& value) noexcept -> shader_program_handle {
    try_set(uniform_location(name), forward<decltype(value)>(value));
    return *this;
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

struct shader_program : shader_program_handle {
 public:
  using base = shader_program_handle;
  using base::handle;

  shader_program() = default;

  shader_program(const vertex_shader& vs,
                 const geometry_shader& gs,
                 const fragment_shader& fs,
                 auto&& warning_handle) {
    receive_handle();
    glAttachShader(handle, vs);
    glAttachShader(handle, gs);
    glAttachShader(handle, fs);
    link(std::forward<decltype(warning_handle)>(warning_handle));
  }
  shader_program(const vertex_shader& vs,
                 const geometry_shader& gs,
                 const fragment_shader& fs)
      : shader_program{vs, gs, fs, warnings_as_errors} {}

  shader_program(const vertex_shader& vs,
                 const fragment_shader& fs,
                 auto&& warning_handle) {
    receive_handle();
    glAttachShader(handle, vs);
    glAttachShader(handle, fs);
    link(std::forward<decltype(warning_handle)>(warning_handle));
  }
  shader_program(const vertex_shader& vs, const fragment_shader& fs)
      : shader_program{vs, fs, warnings_as_errors} {}

  ~shader_program() {
    // Zero values are ignored by this function.
    glDeleteProgram(handle);
  }

  // Copying is not allowed.
  shader_program(const shader_program&) = delete;
  shader_program& operator=(const shader_program&) = delete;

  // Moving
  shader_program(shader_program&& x) : base{x.handle} { x.handle = 0; }
  shader_program& operator=(shader_program&& x) {
    swap(handle, x.handle);
    return *this;
  }

  // operator GLuint() const { return handle; }

  //  void bind() const { glUseProgram(handle); }

  //  bool exists() const { return glIsProgram(handle) == GL_TRUE; }

  //  auto set(czstring name, auto&& value) -> shader_program& {
  //    try_set(valid_uniform_location(name), forward<decltype(value)>(value));
  //    return *this;
  //  }

  //  auto try_set(czstring name, auto&& value) noexcept -> shader_program& {
  //    try_set(uniform_location(name), forward<decltype(value)>(value));
  //    return *this;
  //  }

  // private:
  //  auto uniform_location(czstring name) noexcept {
  //    return glGetUniformLocation(handle, name);
  //  }
  //  auto valid_uniform_location(czstring name) {
  //    const auto result = uniform_location(name);
  //    if (result == -1)
  //      throw invalid_argument(
  //          string("Failed to receive valid uniform location for '") + name +
  //          "'.");
  //    return result;
  //  }

  //  static void try_set(GLint location, float value) noexcept {
  //    glUniform1f(location, value);
  //  }
  //  static void try_set(GLint location, float x, float y) noexcept {
  //    glUniform2f(location, x, y);
  //  }
  //  static void try_set(GLint location, float x, float y, float z) noexcept {
  //    glUniform3f(location, x, y, z);
  //  }
  //  static void try_set(GLint location,
  //                      float x,
  //                      float y,
  //                      float z,
  //                      float w) noexcept {
  //    glUniform4f(location, x, y, z, w);
  //  }

  //  static void try_set(GLint location, GLint value) noexcept {
  //    glUniform1i(location, value);
  //  }
  //  static void try_set(GLint location, GLint x, GLint y) noexcept {
  //    glUniform2i(location, x, y);
  //  }
  //  static void try_set(GLint location, GLint x, GLint y, GLint z) noexcept {
  //    glUniform3i(location, x, y, z);
  //  }
  //  static void try_set(GLint location,
  //                      GLint x,
  //                      GLint y,
  //                      GLint z,
  //                      GLint w) noexcept {
  //    glUniform4i(location, x, y, z, w);
  //  }

  //  static void try_set(GLint location, GLuint value) noexcept {
  //    glUniform1ui(location, value);
  //  }
  //  static void try_set(GLint location, GLuint x, GLuint y) noexcept {
  //    glUniform2ui(location, x, y);
  //  }
  //  static void try_set(GLint location, GLuint x, GLuint y, GLuint z) noexcept {
  //    glUniform3ui(location, x, y, z);
  //  }
  //  static void try_set(GLint location,
  //                      GLuint x,
  //                      GLuint y,
  //                      GLuint z,
  //                      GLuint w) noexcept {
  //    glUniform4ui(location, x, y, z, w);
  //  }

  //  static void try_set(GLint location, const vec2& value) noexcept {
  //    try_set(location, value.x, value.y);
  //  }
  //  static void try_set(GLint location, const vec3& value) noexcept {
  //    try_set(location, value.x, value.y, value.z);
  //  }
  //  static void try_set(GLint location, const vec4& value) noexcept {
  //    try_set(location, value.x, value.y, value.z, value.w);
  //  }

  //  static void try_set(GLint location, const ivec2& value) noexcept {
  //    try_set(location, value.x, value.y);
  //  }
  //  static void try_set(GLint location, const ivec3& value) noexcept {
  //    try_set(location, value.x, value.y, value.z);
  //  }
  //  static void try_set(GLint location, const ivec4& value) noexcept {
  //    try_set(location, value.x, value.y, value.z, value.w);
  //  }

  //  static void try_set(GLint location, const uvec2& value) noexcept {
  //    try_set(location, value.x, value.y);
  //  }
  //  static void try_set(GLint location, const uvec3& value) noexcept {
  //    try_set(location, value.x, value.y, value.z);
  //  }
  //  static void try_set(GLint location, const uvec4& value) noexcept {
  //    try_set(location, value.x, value.y, value.z, value.w);
  //  }

  //  static void try_set(GLint location, const mat2& value) noexcept {
  //    glUniformMatrix2fv(location, 1, GL_FALSE, value_ptr(value));
  //  }
  //  static void try_set(GLint location, const mat3& value) noexcept {
  //    glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(value));
  //  }
  //  static void try_set(GLint location, const mat4& value) noexcept {
  //    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
  //  }

  //  static void try_set(GLint location, const mat2x3& value) noexcept {
  //    glUniformMatrix2x3fv(location, 1, GL_FALSE, value_ptr(value));
  //  }
  //  static void try_set(GLint location, const mat3x2& value) noexcept {
  //    glUniformMatrix3x2fv(location, 1, GL_FALSE, value_ptr(value));
  //  }

  //  static void try_set(GLint location, const mat2x4& value) noexcept {
  //    glUniformMatrix2x4fv(location, 1, GL_FALSE, value_ptr(value));
  //  }
  //  static void try_set(GLint location, const mat4x2& value) noexcept {
  //    glUniformMatrix4x2fv(location, 1, GL_FALSE, value_ptr(value));
  //  }

  //  static void try_set(GLint location, const mat3x4& value) noexcept {
  //    glUniformMatrix3x4fv(location, 1, GL_FALSE, value_ptr(value));
  //  }
  //  static void try_set(GLint location, const mat4x3& value) noexcept {
  //    glUniformMatrix4x3fv(location, 1, GL_FALSE, value_ptr(value));
  //  }

 private:
  void receive_handle() {
    handle = glCreateProgram();
    if (!handle)
      throw runtime_error(string("Failed to receive shader program handle."));
  }

  bool link_failed() noexcept {
    GLint status;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    return static_cast<GLboolean>(status) == GL_FALSE;
  }

  void write_link_info_log(string& info_log) {
    GLint info_log_size;
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &info_log_size);
    info_log.resize(info_log_size);
    if (info_log_size)
      glGetProgramInfoLog(handle, info_log_size, nullptr, info_log.data());
  }

  void throw_link_error() {
    throw shader_link_error("Failed to link shader program.");
  }

  void throw_link_error(const string& info_log) {
    throw shader_link_error("Failed to link shader program.\n" + info_log);
  }

  void link() { glLinkProgram(handle); }

  void link(string& info_log) {
    link();
    write_link_info_log(info_log);
    if (link_failed()) throw_link_error(info_log);
  }

  void link(warnings_as_errors_t) {
    link();
    string info_log;
    write_link_info_log(info_log);
    if (link_failed() || !info_log.empty()) throw_link_error(info_log);
  }

  void link(ignore_warnings_t) {
    link();
    if (link_failed()) {
      string info_log{};
      write_link_info_log(info_log);
      throw_link_error(info_log);
    }
  }

  void link(auto&& warning_callback) {
    string info_log;
    link(info_log);
    if (!info_log.empty())
      std::forward<decltype(warning_callback)>(warning_callback)(info_log);
  }
};

}  // namespace ensketch::opengl
