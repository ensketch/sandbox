#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

namespace detail {

template <typename T>
struct attribute_pointer_traits;
// Builtin Types
template <typename T>
  requires requires { common_enum_value<T>::value; }  //
struct attribute_pointer_traits<T> {
  static constexpr GLenum type = opengl::common_enum_value<T>;
  static constexpr GLint size = 1;
};
// GLM Vector Types
template <typename T, int N, glm::qualifier Q>
struct attribute_pointer_traits<glm::vec<N, T, Q>> {
  static constexpr GLenum type = opengl::common_enum_value<T>;
  static constexpr GLint size = N;
};

// Genericly set a vertex attribute pointer according to a given type.
template <typename T>
  requires requires { attribute_pointer_traits<T>::size; }  //
void set_attribute_pointer(GLuint location,
                           size_t stride,
                           size_t offset) noexcept {
  using traits = attribute_pointer_traits<T>;
  glEnableVertexAttribArray(location);
  glVertexAttribPointer(location, traits::size, traits::type, GL_FALSE, stride,
                        (void*)offset);
};

}  // namespace detail

template <bool binded = false>
struct vertex_array_handle : object_handle {
  using base = object_handle;
  using base::base;

  using binded_handle = vertex_array_handle<true>;
  using unbinded_handle = vertex_array_handle<false>;
  using this_handle = vertex_array_handle<binded>;

  bool valid() const noexcept { return glIsVertexArray(handle) == GL_TRUE; }

  auto bind() const noexcept -> binded_handle {
    if constexpr (!binded) glBindVertexArray(handle);
    return handle;
  }

  static void unbind() noexcept { glBindVertexArray(0); }

  // template <generic::static_layout_tuple tuple_type>
  // auto setup_aos() const noexcept -> binded_handle {
  //   const auto self = bind();

  //   using indices =
  //       meta::static_index_list::iota<std::tuple_size<tuple_type>::value>;

  //   indices::for_each([]<size_t index> {
  //     using type = typename std::tuple_element<index, tuple_type>::type;
  //     detail::set_attribute_pointer<type>(
  //         index, sizeof(tuple_type),
  //         meta::tuple::byte_offset<tuple_type, index>);
  //   });

  //   return self;
  // }
};

class vertex_array : public vertex_array_handle<> {
  using base = vertex_array_handle<>;
  using base::handle;

 public:
  vertex_array() noexcept { glGenVertexArrays(1, &handle); }
  virtual ~vertex_array() noexcept { glDeleteVertexArrays(1, &handle); }

  // Copying is not allowed.
  vertex_array(const vertex_array&) = delete;
  vertex_array& operator=(const vertex_array&) = delete;

  // Moving
  vertex_array(vertex_array&& x) : base{x.handle} { x.handle = 0; }
  vertex_array& operator=(vertex_array&& x) {
    swap(handle, x.handle);
    return *this;
  }
};

}  // namespace ensketch::opengl
