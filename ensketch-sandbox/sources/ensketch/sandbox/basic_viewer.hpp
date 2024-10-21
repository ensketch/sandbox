#pragma once
#include <ensketch/sandbox/log.hpp>
#include <ensketch/sandbox/opengl_window.hpp>

namespace ensketch::sandbox {

///
///
class basic_viewer_state : public opengl_window_state {
  friend opengl_window_state;

 public:
  basic_viewer_state(int width = 500,
                     int height = 500,
                     std::string_view title = "Basic Viewer")
      : opengl_window_state{width, height, title} {
    // int flags;
    // glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    // if (flags & static_cast<int>(GL_CONTEXT_FLAG_DEBUG_BIT)) {
    //   glEnable(GL_DEBUG_OUTPUT);
    //   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //   glDebugMessageCallback(glDebugOutput, nullptr);
    //   glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
    //   nullptr,
    //                         GL_TRUE);
    // }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(10.0f);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  }

 protected:
  void process(const sf::Event& event) {
    if (event.type == sf::Event::Closed)
      sandbox::quit();
    else if (event.type == sf::Event::Resized)
      on_resize(event.size.width, event.size.height);
    else if (event.type == sf::Event::MouseWheelScrolled)
      zoom(0.1 * event.mouseWheelScroll.delta);
    else if (event.type == sf::Event::KeyPressed) {
      switch (event.key.code) {
        case sf::Keyboard::Escape:
          sandbox::quit();
          break;
        case sf::Keyboard::Num1:
          set_y_as_up();
          break;
        case sf::Keyboard::Num2:
          set_z_as_up();
          break;
      }
    }
  }

  void on_resize(int width, int height) {
    glViewport(0, 0, width, height);
    camera.set_screen_resolution(width, height);
    view_should_update = true;
  }

  void turn(const vec2& angle) {
    altitude += angle.y;
    azimuth += angle.x;
    constexpr float bound = pi / 2 - 1e-5f;
    altitude = std::clamp(altitude, -bound, bound);
    view_should_update = true;
  }

  void shift(const vec2& pixels) {
    const auto shift = -pixels.x * camera.right() + pixels.y * camera.up();
    const auto scale = camera.pixel_size() * radius;
    origin += scale * shift;
    view_should_update = true;
  }

  void zoom(float scale) {
    radius *= exp(-scale);
    view_should_update = true;
  }

  // void look_at(float x, float y) {
  //   const auto r = primary_ray(camera, x, y);
  //   if (const auto p = intersection(r, surface)) {
  //     origin = r(p.t);
  //     radius = p.t;
  //     view_should_update = true;
  //   }
  // }

  void set_z_as_up() {
    right = {1, 0, 0};
    front = {0, -1, 0};
    up = {0, 0, 1};
    view_should_update = true;
  }

  void set_y_as_up() {
    right = {1, 0, 0};
    front = {0, 0, 1};
    up = {0, 1, 0};
    view_should_update = true;
  }

  void update_view() {
    // Compute camera position by using spherical coordinates.
    // This transformation is a variation of the standard
    // called horizontal coordinates often used in astronomy.
    //
    auto p = cos(altitude) * sin(azimuth) * right +  //
             cos(altitude) * cos(azimuth) * front +  //
             sin(altitude) * up;
    p *= radius;
    p += origin;
    camera.move(p).look_at(origin, up);

    // camera.set_near_and_far(std::max(1e-3f * radius, radius -
    // bounding_radius),
    //                         radius + bounding_radius);
  }

  void render() {
    // Get new mouse position and compute movement in space.
    const auto new_mouse_pos = mouse_position();
    const auto mouse_move = new_mouse_pos - mouse_pos;
    mouse_pos = new_mouse_pos;
    if (focused()) {
      if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
          shift({mouse_move.x, mouse_move.y});
        else
          turn({-0.01 * mouse_move.x, 0.01 * mouse_move.y});
      }
    }

    if (view_should_update) {
      update_view();
      view_should_update = false;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  // Stores updated mouse position in window coordinates.
  //
  sf::Vector2i mouse_pos{};

  //
  bool view_should_update = false;

  // 3D Coordinate System for Viewing
  //
  // World Origin
  vec3 origin;
  // Basis Vectors of Right-Handed Coordinate System
  vec3 up{0, 1, 0};
  vec3 right{1, 0, 0};
  vec3 front{0, 0, 1};
  // Spherical/Horizontal Coordinates of Camera
  float radius = 10;
  float altitude = 0;
  float azimuth = 0;

  // Perspective camera
  //
  opengl::camera camera{};
};

///
///
template <typename derived>
struct basic_viewer_api : opengl_window_api<derived> {
  using base = opengl_window_api<derived>;
  using state_type = basic_viewer_state;

  using base::self;

  void set_background_color(float red, float green, float blue) {
    self().invoke([red, green, blue](state_type& state) {
      glClearColor(red, green, blue, 1.0f);
    });
  }
};

}  // namespace ensketch::sandbox
