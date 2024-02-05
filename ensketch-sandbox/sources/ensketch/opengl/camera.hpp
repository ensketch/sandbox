#pragma once
#include <ensketch/opengl/utility.hpp>

namespace ensketch::opengl {

// GLM already provides functions to construct the view and projection matrix.
// So, what is the exact purpose of the camera?
// It enables user interaction by providing easy access to up and right vectors
// and to the pixel size.
// This way, we have a proper way to combine
// seamlessly ray tracing with rasterization.
//
// The tripod should actually be used as a base class
// for further specialization of cameras.
// Tripod provides look_at, rotate in four variations,
// translate in four variations, and construction of view and inverse view matrix.
//
// For now, I will remove all caching from the camera until a better design pops up.

class camera {
 public:
  constexpr camera() noexcept = default;

  constexpr auto right() const noexcept { return x; }
  constexpr auto up() const noexcept { return y; }
  constexpr auto front() const noexcept { return z; }
  constexpr auto position() const noexcept { return o; }

  constexpr auto direction() const noexcept { return -z; }

  constexpr auto vfov() const noexcept { return fov; }
  constexpr auto hfov() const noexcept {
    return 2.0f * atan(tan(0.5f * fov) * pixels.x / pixels.y);
  }

  constexpr auto screen_width() const noexcept { return pixels.x; }
  constexpr auto screen_height() const noexcept { return pixels.y; }

  constexpr auto near() const noexcept { return z_min; }
  constexpr auto far() const noexcept { return z_max; }

  constexpr auto aspect_ratio() const noexcept {
    return float(pixels.x) / pixels.y;
  }

  constexpr auto pixel_size() const noexcept {
    return 2.0f * tan(0.5f * vfov()) / pixels.y;
  }

  auto view_matrix() const noexcept {
    return lookAt(position(), position() + direction(), up());
  }

  auto projection_matrix() const noexcept {
    return glm::perspective(vfov(), aspect_ratio(), near(), far());
  }

  auto viewport_matrix() const noexcept {
    return scale(  //
        mat4{1.0f}, {screen_width() / 2.0f, screen_height() / 2.0f, 1.0f});
  }

  // auto primary_ray(float x, float y) const noexcept -> ray {
  //   return ray{position(),
  //              normalize(direction() +
  //                        pixel_size() * ((x - 0.5f * screen_width()) * right() +
  //                                        (0.5f * screen_height() - y) * up()))};
  // }

  constexpr auto set_screen_resolution(int w, int h) noexcept -> camera& {
    pixels.x = w;
    pixels.y = h;
    return *this;
  }

  constexpr auto set_vfov(float radians) noexcept -> camera& {
    fov = radians;
    return *this;
  }

  constexpr auto set_hfov(float radians) noexcept -> camera& {
    fov = 2.0f * atan(tan(0.5f * radians) * pixels.y / pixels.x);
    return *this;
  }

  constexpr auto set_near_and_far(float n, float f) noexcept -> camera& {
    z_min = n;
    z_max = f;
    return *this;
  }

  constexpr auto move(const vec3& t) noexcept -> camera& {
    o = t;
    return *this;
  }

  auto look_at(const vec3& focus, const vec3& rel_up) noexcept -> camera& {
    z = normalize(o - focus);
    x = normalize(cross(rel_up, z));
    y = normalize(cross(z, x));
    return *this;
  }

 private:
  vec3 x{1, 0, 0};
  vec3 y{0, 1, 0};
  vec3 z{0, 0, 1};
  vec3 o{0, 0, 0};

  ivec2 pixels{500, 500};
  float fov = pi / 4;

  float z_min = 0.1f;
  float z_max = 1000.0f;
};

}  // namespace ensketch::opengl
