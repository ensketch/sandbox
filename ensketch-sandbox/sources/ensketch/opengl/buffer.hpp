#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

// struct buffer_handle : object_handle {
//   using base = object_handle;
//   using base::base;

//   bool valid() const noexcept { return glIsBuffer(handle) == GL_TRUE; }

//   operator bool() const noexcept { return valid(); }

//   auto size() const noexcept -> size_t {
//     GLint s;
//     glGetNamedBufferParameteriv(handle, GL_BUFFER_SIZE, &s);
//     return s;
//   }
// };

// this one will explain all custom typed functions to be used on the handle
//
template <auto buffer_type>
struct buffer_handle : object_handle {
  using base = object_handle;
  using base::base;

  using this_handle = buffer_handle<buffer_type>;

  bool valid() const noexcept { return glIsBuffer(handle) == GL_TRUE; }

  void bind() const noexcept { glBindBuffer(buffer_type, handle); }

  static void unbind() noexcept { glBindBuffer(buffer_type, 0); }

  auto size() const noexcept -> size_t {
    GLint s;
    glGetNamedBufferParameteriv(handle, GL_BUFFER_SIZE, &s);
    return s;
  }

  void allocate_and_initialize(const void* data, size_t size) const noexcept {
    glNamedBufferData(id(), size, data, GL_STATIC_DRAW);
  }

  void allocate_and_initialize(const auto* data, size_t size) const noexcept {
    allocate_and_initialize(static_cast<const void*>(data),
                            size * sizeof(data[0]));
  }

  template <generic::transferable type>
    requires(!ranges::contiguous_range<type>)  //
  void allocate_and_initialize(const type& value) const noexcept {
    allocate_and_initialize(&value, 1);
  }

  void allocate_and_initialize(
      const ranges::contiguous_range auto& range) const noexcept {
    allocate_and_initialize(ranges::data(range), ranges::size(range));
  }

  void allocate(size_t size) const noexcept {
    allocate_and_initialize(static_cast<const void*>(nullptr), size);
  }

  void write(const void* data, size_t size, size_t offset = 0) const noexcept {
    // assert(offset + size <= self.size());
    glBufferSubData(buffer_type, offset, size, data);
  }

  void write(const auto* data, size_t size, size_t offset = 0) const noexcept {
    write(static_cast<const void*>(data),  //
          size * sizeof(data[0]), offset);
  }

  void write(const ranges::contiguous_range auto& range,
             size_t offset = 0) const noexcept {
    write(ranges::data(range), ranges::size(range), offset);
  }

  void write(const generic::transferable auto& v,
             size_t offset = 0) const noexcept {
    write(&v, 1, offset);
  }

  void set_binding(GLuint index) const noexcept  //
    requires((buffer_type == GL_ATOMIC_COUNTER_BUFFER) ||
             (buffer_type == GL_TRANSFORM_FEEDBACK_BUFFER) ||
             (buffer_type == GL_UNIFORM_BUFFER) ||
             (buffer_type == GL_SHADER_STORAGE_BUFFER))
  {
    // Function automatically binds the buffer.
    glBindBufferBase(buffer_type, index, handle);
  }
};

// the actual container stored on the device
// it manages creation and deletion
template <auto buffer_type>
class buffer final : public buffer_handle<buffer_type> {
  using base = buffer_handle<buffer_type>;
  using base::handle;

 public:
  buffer() noexcept { glCreateBuffers(1, &handle); }

  ~buffer() noexcept { glDeleteBuffers(1, &handle); }

  // Copying is NOT allowed.
  //
  buffer(const buffer&) = delete;
  buffer& operator=(const buffer&) = delete;

  // Moving is allowed.
  //
  buffer(buffer&& x) : base{x.handle} { x.handle = 0; }
  buffer& operator=(buffer&& x) {
    swap(handle, x.handle);
    return *this;
  }
};

using vertex_buffer = buffer<GL_ARRAY_BUFFER>;
using element_buffer = buffer<GL_ELEMENT_ARRAY_BUFFER>;
using uniform_buffer = buffer<GL_UNIFORM_BUFFER>;
using shader_storage_buffer = buffer<GL_SHADER_STORAGE_BUFFER>;

}  // namespace ensketch::opengl
