#pragma once
#include <ensketch/opengl/opengl.hpp>
//
#include <SFML/Graphics.hpp>
//
#include <ensketch/sandbox/polyhedral_surface.hpp>
//
#include <geometrycentral/surface/edge_length_geometry.h>
#include <geometrycentral/surface/manifold_surface_mesh.h>
#include <geometrycentral/surface/vertex_position_geometry.h>
//
#include <igl/heat_geodesics.h>

namespace ensketch::sandbox {

using namespace gl;

class viewer {
 public:
  static sf::ContextSettings opengl_context_settings;

  viewer() noexcept {}

  void open(int width, int height);
  void close();

  bool is_open() const noexcept { return window.isOpen(); }
  operator bool() const noexcept { return is_open(); }

  void init();
  void free();

  void resize(int width, int height);
  void process_events();
  void update();
  void update_view();
  void render();

  void set_view_should_update() noexcept;

  void turn(const vec2& angle);
  void shift(const vec2& pixels);
  void zoom(float scale);
  void look_at(float x, float y);

  void set_z_as_up();
  void set_y_as_up();

  void load_surface(const filesystem::path& path);
  void async_load_surface(const filesystem::path& path);
  void handle_surface_load_task();

  void fit_view();
  void print_surface_info();

  bool running() const noexcept { return _running; }

  void set_vsync(bool value = true) noexcept {
    window.setVerticalSyncEnabled(value);
  }

  auto mouse_to_vertex(float x, float y) noexcept
      -> polyhedral_surface::vertex_id;

  void select_vertex(float x, float y) noexcept;

  void record_mouse_curve() noexcept;
  void reset_mouse_curve() noexcept;

  void project_mouse_curve();

  void compute_topology_and_geometry();

  void regularize_open_surface_vertex_curve();
  void regularize_closed_surface_vertex_curve();
  void close_surface_vertex_curve();
  void mouse_append_surface_vertex_curve(float x, float y);
  void regular_append_to_surface_vertex_curve(
      polyhedral_surface::vertex_id vid);
  void reset_surface_vertex_curve();

  void compute_geodesic();

  void compute_heat_data();
  void update_heat();

  void compute_smooth_line();

 private:
  bool _running = false;
  sf::Window window{};

  sf::Vector2i mouse_pos{};

  //
  bool view_should_update = false;

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

  opengl::camera camera{};

  struct device_storage {
    opengl::shader_program shader{};
    opengl::vertex_array va{};
    opengl::vertex_buffer vertices{};
    opengl::element_buffer faces{};

    opengl::shader_program point_shader{};
    opengl::element_buffer selected_vertices{};

    opengl::shader_program surface_vertex_curve_shader{};
    opengl::element_buffer surface_vertex_curve_data{};

    opengl::vertex_array surface_mesh_curve_va{};
    opengl::vertex_buffer surface_mesh_curve_data{};

    opengl::shader_program mouse_curve_shader{};
    opengl::vertex_array mouse_curve_va{};
    opengl::vertex_buffer mouse_curve_data{};
  };
  optional<device_storage> device{};

  polyhedral_surface surface{};
  // scene surface{};
  bool surface_should_update = false;
  // The loading of mesh data can take quite a long time
  // and may let the window manager think the program is frozen
  // if the data would be loaded by a blocking call.
  // Here, an asynchronous task is used
  // to get rid of this unresponsiveness.
  future<void> surface_load_task{};
  float32 surface_load_time{};
  float32 surface_process_time{};
  //
  float bounding_radius;

  polyhedral_surface::vertex_id selected_vertex = polyhedral_surface::invalid;

  vector<vec2> mouse_curve{};
  bool mouse_curve_recording = false;

  vector<polyhedral_surface::vertex_id> surface_vertex_curve{};
  bool surface_vertex_curve_closed = false;

  unique_ptr<geometrycentral::surface::ManifoldSurfaceMesh> mesh{};
  unique_ptr<geometrycentral::surface::VertexPositionGeometry> geometry{};

  vector<vec3> surface_mesh_curve{};

  // Heat Geodsics from libigl
  //
  Eigen::MatrixXd surface_vertex_matrix;
  Eigen::MatrixXi surface_face_matrix;
  igl::HeatGeodesicsData<double> heat_data;
  Eigen::VectorXd heat;
  // opengl::vertex_buffer device_heat{};
  vector<float> potential;
  //
  unique_ptr<geometrycentral::surface::EdgeLengthGeometry> lifted_geometry{};
  float avg_edge_length = 1.0f;
  float tolerance = 2.0f;
  float bound = 1.0f;
  float transition = 1.0f;
};

}  // namespace ensketch::sandbox
