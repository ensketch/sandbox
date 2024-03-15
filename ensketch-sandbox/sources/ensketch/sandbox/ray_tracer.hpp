#pragma once
#include <ensketch/sandbox/polyhedral_surface.hpp>
#include <ensketch/sandbox/utility.hpp>
//
#include <ensketch/opengl/camera.hpp>

namespace ensketch::sandbox {

struct ray {
  auto operator()(float t) const noexcept { return origin + t * direction; }

  vec3 origin;
  vec3 direction;
};

struct triangle : array<vec3, 3> {};

struct ray_triangle_intersection {
  operator bool() const noexcept {
    return (u >= 0.0f) && (v >= 0.0f) && (u + v <= 1.0) && (t > 0.0f);
  }

  float u{};
  float v{};
  float t{};
};

auto intersection(const ray& r, const triangle& f) noexcept
    -> ray_triangle_intersection;

struct ray_polyhedral_surface_intersection : ray_triangle_intersection {
  // We overwrite the check,
  // because it the triangle will already have been checked.
  operator bool() const noexcept { return f != -1; }
  uint32_t f = -1;
};

auto intersection(const ray& r, const polyhedral_surface& scene) noexcept
    -> ray_polyhedral_surface_intersection;

inline auto primary_ray(const opengl::camera& camera, float x, float y) noexcept
    -> ray {
  return ray{
      camera.position(),
      normalize(camera.direction() +
                camera.pixel_size() *
                    ((x - 0.5f * camera.screen_width()) * camera.right() +
                     (0.5f * camera.screen_height() - y) * camera.up()))};
}

}  // namespace ensketch::sandbox
