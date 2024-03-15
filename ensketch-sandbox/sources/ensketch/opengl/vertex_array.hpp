#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

struct vertex_array_handle : object_handle {
  using base = object_handle;
  using base::base;

  bool valid() const noexcept { return glIsVertexArray(handle) == GL_TRUE; }

  operator bool() const noexcept { return valid(); }

  void bind() const noexcept { glBindVertexArray(handle); }

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

class vertex_array final : public vertex_array_handle {
  using base = vertex_array_handle;
  // using base::handle;

 public:
  vertex_array() noexcept { glGenVertexArrays(1, &handle); }

  ~vertex_array() noexcept { glDeleteVertexArrays(1, &handle); }

  // Copying is NOT allowed.
  //
  vertex_array(const vertex_array&) = delete;
  vertex_array& operator=(const vertex_array&) = delete;

  // Moving is allowed.
  //
  vertex_array(vertex_array&& x) : base{x.handle} { x.handle = 0; }
  vertex_array& operator=(vertex_array&& x) {
    swap(handle, x.handle);
    return *this;
  }
};

}  // namespace ensketch::opengl
