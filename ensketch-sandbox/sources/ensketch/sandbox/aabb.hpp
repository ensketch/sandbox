#pragma once
#include <ensketch/sandbox/utility.hpp>

namespace ensketch::sandbox {

namespace generic {
template <typename type>
concept point = same_as<type, vec2> || same_as<type, vec3>;
}  // namespace generic

template <generic::point point>
struct aabb {
  using point_type = point;
  using real = typename point::value_type;

  aabb() = default;
  constexpr aabb(const point_type& p) noexcept : _min{p}, _max{p} {}
  constexpr aabb(const point_type& p, const point_type& q) noexcept
      : _min{min(p, q)}, _max{max(p, q)} {}
  constexpr aabb(const aabb& a, const aabb& b) noexcept
      : _min{min(a._min, b._min)}, _max{max(a._max, b._max)} {}
  constexpr aabb(const aabb& a, const point_type& p) noexcept
      : _min{min(a._min, p)}, _max{max(a._max, p)} {}

  // constexpr auto min() const noexcept -> point_type { return _min; }
  // constexpr auto max() const noexcept -> point_type { return _max; }

  constexpr auto origin() const noexcept -> point_type {
    return (_min + _max) / real(2);
  }

  constexpr auto radius() const noexcept -> real {
    return length(_max - _min) / real(2);
  }

  point_type _min{};
  point_type _max{};
};

namespace detail {
template <typename type>
struct is_aabb : false_type {};
template <typename point>
struct is_aabb<aabb<point>> : true_type {};
}  // namespace detail

namespace instance {
template <typename type>
concept aabb = detail::is_aabb<type>::value;
}  // namespace instance

// AABB for two and three dimensions are the most common use case.
//
using aabb2 = aabb<vec2>;
using aabb3 = aabb<vec3>;

/// All constructors of 'aabb' can also be used with constructor extensions.
///
ENSKETCH_SANDBOX_ADD_DEFAULT_CONSTRUCTOR_EXTENSION(aabb);

/// Construct an AABB around a range of given points.
///
template <ranges::input_range range_type>
constexpr auto aabb_from(const range_type& range) noexcept {
  using aabb_type = aabb<ranges::range_value_t<range_type>>;
  if (ranges::empty(range)) return aabb_type{};
  auto it = ranges::begin(range);
  // aabb result{*it++};
  auto result = aabb_from(*it++);
  for (; it != ranges::end(range); ++it) result = aabb(result, *it);
  return result;
}

}  // namespace ensketch::sandbox
