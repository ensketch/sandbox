#include <ensketch/luarepl/luarepl.hpp>
#include <ensketch/sandbox/log.hpp>
#include <ensketch/sandbox/simple_viewer.hpp>

namespace ensketch::sandbox {

template <meta::string str, meta::string docstr, typename functor>
struct function_entry : functor {
  using base = functor;
  using base::operator();
  static consteval auto name() noexcept { return str; }
  static consteval auto docstring() noexcept { return docstr; }
};

template <meta::string name, meta::string docstr>
constexpr auto fn(auto&& f) noexcept {
  return function_entry<name, docstr, std::unwrap_ref_decay_t<decltype(f)>>{
      std::forward<decltype(f)>(f)};
}

template <typename type>
concept function_entry_instance =
    matches<type,
            []<meta::string name, meta::string docstr, typename functor>(
                function_entry<name, docstr, functor>) {
              return meta::as_signature<true>;
            }>;

template <typename... types>
struct function_list : std::tuple<types...> {
  using base = std::tuple<types...>;
  using base::base;
};

template <typename... types>
constexpr auto function_list_from(types... values) {
  return named_tuple<meta::name_list<types::name()...>, std::tuple<types...>>{
      values...};
}

void add_lua_functions() {
  static constexpr auto functions = std::tuple{
      fn<"quit", "Quit the application.">([] { quit(); }),
      fn<"print", "Print a given string to the REPL log.">(
          [](czstring str) { luarepl::log(str); }),

      // fn<"open_viewer", "Open the viewer with an OpenGL context.">(
      //     [](int width, int height) { open_viewer(width, height); }),

      // fn<"close_viewer", "Close the viewer.">([] { close_viewer(); }),

      // fn<"screenshot", "Stores the viewer's current framebuffer as image.">(
      //     [](const string& path) {
      //       auto task = main_task_queue().push(
      //           [path] { main_viewer().store_image_from_view(path); });
      //       task.wait();
      //     }),

      // fn<"load_surface", "Load a surface mesh from file.">(
      //     [](const string& path) {
      //       auto task = main_task_queue().push(
      //           [path] { main_viewer().load_surface(path); });
      //       task.wait();
      //     }),

      // fn<"set_wireframe", "Turn on/off wireframe for shading.">([](bool value) {
      //   auto task =
      //       main_task_queue().push([=] { main_viewer().set_wireframe(value); });
      //   task.wait();
      // }),

      // fn<"use_face_normal", "Turn on/off face normals for shading.">(
      //     [](bool value) {
      //       auto task = main_task_queue().push(
      //           [=] { main_viewer().use_face_normal(value); });
      //       task.wait();
      //     }),

      // fn<"save_perspective", "Save camera perspective to given file.">(
      //     [](const string& path) { main_viewer().save_perspective(path); }),

      // fn<"load_perspective", "Load camera perspective from given file.">(
      //     [](const string& path) {
      //       auto task = main_task_queue().push(
      //           [path] { main_viewer().load_perspective(path); });
      //       task.wait();
      //     }),

      // fn<"save_surface_vertex_curve",
      //    "Save current surface vertex curve to given file.">(
      //     [](const string& path) {
      //       main_viewer().save_surface_vertex_curve(path);
      //     }),

      // fn<"load_surface_vertex_curve",
      //    "Load surface vertex curve from given file.">([](const string& path) {
      //   auto task = main_task_queue().push(
      //       [path] { main_viewer().load_surface_vertex_curve(path); });
      //   task.wait();
      // }),

      // fn<"smooth_surface_vertex_curve",
      //    "Smooth the current surface vertex curve by using the geodetic "
      //    "smoothing approach.">([](double tolerance, int laplace_iterations,
      //                              double laplace_relaxation) {
      //   auto task = main_task_queue().push([=] {
      //     auto& viewer = main_viewer();
      //     viewer.tolerance = tolerance;
      //     viewer.laplace_iterations = laplace_iterations;
      //     viewer.laplace_relaxation = laplace_relaxation;
      //     viewer.compute_smooth_surface_mesh_curve();
      //   });
      //   task.wait();
      // }),

      // fn<"hyper_smooth_surface_vertex_curve",
      //    "Smooth the current surface vertex curve by using the hyper surface "
      //    "smoothing approach.">([](double lambda, int smoothing_passes) {
      //   auto task = main_task_queue().push([=] {
      //     auto& viewer = main_viewer();
      //     viewer.hyper_lambda = lambda;
      //     viewer.hyper_smoothing_passes = smoothing_passes;
      //     viewer.compute_hyper_surface_smoothing();
      //   });
      //   task.wait();
      // }),
  };

  for_each(functions, [](auto& f) {
    using entry = std::decay_t<decltype(f)>;
    luarepl::set_function(view_from(entry::name()), f);
  });

  luarepl::set_function("help", [] {
    string out{};
    for_each(functions, [&out](auto& f) {
      using entry = std::decay_t<decltype(f)>;
      out += std::format("{}\n{}\n\n", entry::name(), entry::docstring());
    });
    log::info(out);
  });

  auto state = luarepl::lua_state();
  auto table = state["ensketch"].get_or_create<sol::table>();
  for_each(functions, [&table](auto& f) {
    using entry = std::decay_t<decltype(f)>;
    table.set_function(view_from(entry::name()), f);
  });
  table.new_usertype<simple_viewer>(
      "viewer",                                                              //
      "new", sol::constructors<simple_viewer(), simple_viewer(int, int)>{},  //
      "quit",
      &simple_viewer::quit,  //
      "set_background_color", &simple_viewer::set_background_color);
  // table["viewer"].set_function("open", [](int width, int height) {
  //   return simple_viewer{width, height};
  // });
}

}  // namespace ensketch::sandbox
