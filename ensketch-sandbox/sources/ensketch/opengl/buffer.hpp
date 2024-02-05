#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

// this one will explain all custom typed functions to be used on the handle
//
template <auto buffer_type, bool binded = false>
struct buffer_handle : object_handle {
  using base = object_handle;
  using base::base;

  using binded_handle = buffer_handle<buffer_type, true>;
  using unbinded_handle = buffer_handle<buffer_type, false>;
  using this_handle = buffer_handle<buffer_type, binded>;

  bool valid() const noexcept { return glIsBuffer(handle) == GL_TRUE; }

  auto bind() const noexcept -> binded_handle {
    if constexpr (!binded) glBindBuffer(buffer_type, handle);
    return handle;
  }

  static void unbind() noexcept { glBindBuffer(buffer_type, 0); }

  auto size() const noexcept -> size_t {
    const auto self = bind();
    GLint s;
    glGetBufferParameteriv(buffer_type, GL_BUFFER_SIZE, &s);
    return s;
  }

  auto allocate_and_initialize(const void* data, size_t size) const noexcept
      -> binded_handle {
    const auto self = bind();
    glBufferData(buffer_type, size, data, GL_STATIC_DRAW);
    return self;
  }

  auto allocate_and_initialize(const auto* data, size_t size) const noexcept
      -> binded_handle {
    return allocate_and_initialize(static_cast<const void*>(data),
                                   size * sizeof(data[0]));
  }

  template <generic::transferable type>
    requires(!ranges::contiguous_range<type>)  //
  auto allocate_and_initialize(const type& value) const noexcept
      -> binded_handle {
    return allocate_and_initialize(&value, 1);
  }

  auto allocate_and_initialize(const ranges::contiguous_range auto& range)
      const noexcept -> binded_handle {
    return allocate_and_initialize(ranges::data(range), ranges::size(range));
  }

  auto allocate(size_t size) const noexcept -> binded_handle {
    return allocate_and_initialize(static_cast<const void*>(nullptr), size);
  }

  auto write(const void* data, size_t size, size_t offset = 0) const noexcept
      -> binded_handle {
    const auto self = bind();
    assert(offset + size <= self.size());
    glBufferSubData(buffer_type, offset, size, data);
    return self;
  }

  auto write(const auto* data, size_t size, size_t offset = 0) const noexcept
      -> binded_handle {
    return write(static_cast<const void*>(data),  //
                 size * sizeof(data[0]), offset);
  }

  auto write(const ranges::contiguous_range auto& range,
             size_t offset = 0) const noexcept -> binded_handle {
    return write(ranges::data(range), ranges::size(range), offset);
  }

  auto write(const generic::transferable auto& v,
             size_t offset = 0) const noexcept -> binded_handle {
    return write(&v, 1, offset);
  }

  auto set_binding(GLuint index) const noexcept -> binded_handle  //
    requires((buffer_type == GL_ATOMIC_COUNTER_BUFFER) ||
             (buffer_type == GL_TRANSFORM_FEEDBACK_BUFFER) ||
             (buffer_type == GL_UNIFORM_BUFFER) ||
             (buffer_type == GL_SHADER_STORAGE_BUFFER))
  {
    // Function automatically binds the buffer.
    glBindBufferBase(buffer_type, index, handle);
    return handle;
  }
};

// the actual container stored on the device
// it manages creation and deletion
template <auto buffer_type>
class buffer : public buffer_handle<buffer_type> {
  using base = buffer_handle<buffer_type>;
  using base::handle;

 public:
  buffer() noexcept { glGenBuffers(1, &handle); }
  virtual ~buffer() noexcept { glDeleteBuffers(1, &handle); }

  // Copying is not allowed.
  buffer(const buffer&) = delete;
  buffer& operator=(const buffer&) = delete;

  // Moving
  buffer(buffer&& x) : base{x.handle} { x.handle = 0; }
  buffer& operator=(buffer&& x) {
    swap(handle, x.handle);
    return *this;
  }
};

using vertex_buffer = buffer<GL_ARRAY_BUFFER>;
using element_buffer = buffer<GL_ELEMENT_ARRAY_BUFFER>;
using uniform_buffer = buffer<GL_UNIFORM_BUFFER>;

}  // namespace ensketch::opengl
