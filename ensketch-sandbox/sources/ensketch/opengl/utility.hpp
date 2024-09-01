#pragma once
//
#include <ensketch/xstd/xstd.hpp>
//
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
//
#include <glm/glm.hpp>
//
#include <glm/ext.hpp>
//
#include <glm/gtx/norm.hpp>

namespace ensketch::opengl {

using namespace std;
using namespace gl;
using namespace glm;
using namespace xstd;

namespace generic {

template <typename type>
concept transferable = regular<type> && is_trivially_copyable_v<type>;

// Similar to builtin types. Size should be one of a builtin type.
//
template <typename type>
concept simple_value =
    transferable<type> /* && using_by_value_is_fastest<type> */;

}  // namespace generic

// Need uniform interface to handle variable.
// Need to inherit from standards.
// So, the wrapper of a handle is useful.
//
template <generic::simple_value T>
struct basic_handle {
  using type = T;

  constexpr basic_handle() noexcept = default;
  explicit constexpr basic_handle(type value) noexcept : handle{value} {}
  explicit constexpr operator type() noexcept { return handle; }
  constexpr auto id() const noexcept -> type { return handle; }

 protected:
  type handle{};
};

using object_handle = basic_handle<GLuint>;

// constexpr auto ref_cast(auto&& x) noexcept {
//   return std::forward<decltype(x)>(x).ref();
// }

// constexpr auto const_ref_cast(const auto& x) noexcept {
//   return x.ref();
// }

// namespace detail {

// template <typename type>
// struct ref {
//   using type = type&;
// };

// template <typename type>
// struct const_ref {
//   using type = const type&
// };

// template <typename type>
// struct ref<type&> {
//   using type = typename ref<type>::type;
// };

// template <typename type>
// struct ref<const type&> {
//   using type = typename ref<type>::type;
// };

// template <typename type>
// struct ref<ref<type>> {
//   using type = typename ref<type>::type;
// };

// template <typename type>
// struct ref<const_ref<type>> {
//   using type = typename const_ref<type>::type;
// };

// template <typename type>
// struct const_ref<ref<type>> {
//   using type = typename const_ref<type>::type;
// };

// template <typename type>
// struct const_ref<const_ref<type>> {
//   using type = typename const_ref<type>::type;
// };

// }  // namespace detail

// template <typename type>
// using ref = typename detail::ref<type>::type;
// template <typename type>
// using const_ref = typename detail::const_ref<type>::type;

// template <typename type>
// using const_ref = decltype(const_ref_cast(declval<type>()));

// Why reference?
// Only using the handle itself will not allow you to use custom typed functions.
// The handle itself will never statically know about bindings.
// reference template will only store the handle without being able to delete it
// base class should be a handle class
// no default constructor allowed
// but default destructor is mandatory
// due to inheritance, every function from the base handle can be called
//
// template <typename T>
// struct reference : T::base {
//   using base = typename T::base;
//   reference(T& t) : base{t} {}
// };

// Meta functions to transfer builtin types to their common OpenGL enum value.
namespace detail {
template <typename T>
struct common_enum_value {};
template <>
struct common_enum_value<GLbyte> {
  static constexpr GLenum value = GL_BYTE;
};
template <>
struct common_enum_value<GLubyte> {
  static constexpr GLenum value = GL_UNSIGNED_BYTE;
};
template <>
struct common_enum_value<GLshort> {
  static constexpr GLenum value = GL_SHORT;
};
template <>
struct common_enum_value<GLushort> {
  static constexpr GLenum value = GL_UNSIGNED_SHORT;
};
template <>
struct common_enum_value<GLint> {
  static constexpr GLenum value = GL_INT;
};
template <>
struct common_enum_value<GLuint> {
  static constexpr GLenum value = GL_UNSIGNED_INT;
};
template <>
struct common_enum_value<GLfloat> {
  static constexpr GLenum value = GL_FLOAT;
};
template <>
struct common_enum_value<GLdouble> {
  static constexpr GLenum value = GL_DOUBLE;
};
}  // namespace detail

/// Meta function to transfer builtin types to their common OpenGL enum value.
template <typename type>
constexpr auto common_enum_value = detail::common_enum_value<type>::value;

// Meta functions to transfer OpenGL enum value to its common type.
namespace detail {
template <GLenum value>
struct common_type {};
template <>
struct common_type<GL_BYTE> {
  using type = GLbyte;
};
template <>
struct common_type<GL_UNSIGNED_BYTE> {
  using type = GLubyte;
};
template <>
struct common_type<GL_SHORT> {
  using type = GLshort;
};
template <>
struct common_type<GL_UNSIGNED_SHORT> {
  using type = GLushort;
};
template <>
struct common_type<GL_INT> {
  using type = GLint;
};
template <>
struct common_type<GL_UNSIGNED_INT> {
  using type = GLuint;
};
template <>
struct common_type<GL_FLOAT> {
  using type = GLfloat;
};
template <>
struct common_type<GL_DOUBLE> {
  using type = GLdouble;
};
}  // namespace detail

/// Meta function to transfer OpenGL enum value to its common type.
template <GLenum value>
using common_type = typename detail::common_type<value>::type;

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

}  // namespace ensketch::opengl
