#include <ensketch/sandbox/main.hpp>
//
#include <ensketch/sandbox/log.hpp>
#include <ensketch/sandbox/lua.hpp>
//
#include <ensketch/xstd/named_tuple.hpp>

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
  static constexpr auto functions = function_list_from(  //
      fn<"quit", "Quit the application.">([] { quit(); }),

      fn<"open_viewer", "Open the viewer with an OpenGL context.">(
          [](int width, int height) { open_viewer(width, height); }),

      fn<"close_viewer", "Close the viewer.">([] { close_viewer(); }),

      fn<"load_surface", "Load a surface mesh from file.">(
          [](const string& path) {
            auto task = main_task_queue().result_from_push(
                [path] { main_viewer().load_surface(path); });
            task.wait();
          })  //
  );

  for_each(functions, []<meta::string name>(auto& function) {
    lua::add(czstring(name), function);
  });
  lua::add("help", [&functions] {
    string out{};
    for_each(functions, [&out]<meta::string name>(auto& f) {
      out += std::format("{}\n\t{}\n", name, f.docstring());
    });
    log::info(out);
  });

  // Construct a static vector of all lua functions.
  // It needs to be static as the help function will need to refer to it.
  //
  // static vector<tuple<string, string, function_type>> functions{
  //     {"help",                        //
  //      "Print available functions.",  //
  //      fun([&] {
  //        stringstream out{};
  //        out << "Available Functions:\n";
  //        for (const auto& [name, help, data] : functions)
  //          out << '\t' << name << '\n' << "\t\t" << help << "\n\n";
  //        log::info(out.str());
  //      })},

  //     {"quit",                   //
  //      "Quit the application.",  //
  //      fun([] { quit(); })},

  //     {"open_viewer",                              //
  //      "Open the viewer with an OpenGL context.",  //
  //      fun([](int width, int height) { open_viewer(width, height); })},

  //     {"close_viewer",                          //
  //      "Close the viewer and OpenGL context.",  //
  //      fun([] { close_viewer(); })},

  //     {"screenshot",                                         //
  //      "Stores the viewer's current framebuffer as image.",  //
  //      fun([](const string& path) {
  //        auto task = main_task_queue().result_from_push(
  //            [path] { main_viewer().store_image_from_view(path); });
  //        task.wait();
  //      })},

  //     {"load_surface",                    //
  //      "Load a surface mesh from file.",  //
  //      fun([](const string& path) {
  //        auto task = main_task_queue().result_from_push(
  //            [path] { main_viewer().load_surface(path); });
  //        task.wait();
  //      })},

  //     {"save_perspective",                        //
  //      "Save camera perspective to given file.",  //
  //      fun([](const string& path) { main_viewer().save_perspective(path); })},

  //     {"load_perspective",                          //
  //      "Load camera perspective from given file.",  //
  //      fun([](const string& path) {
  //        auto task = main_task_queue().result_from_push(
  //            [path] { main_viewer().load_perspective(path); });
  //        task.wait();
  //      })},

  //     {"save_surface_vertex_curve",                         //
  //      "Save current surface vertex curve to given file.",  //
  //      fun([](const string& path) {
  //        main_viewer().save_surface_vertex_curve(path);
  //      })},

  //     {"load_surface_vertex_curve",                   //
  //      "Load surface vertex curve from given file.",  //
  //      fun([](const string& path) {
  //        auto task = main_task_queue().result_from_push(
  //            [path] { main_viewer().load_surface_vertex_curve(path); });
  //        task.wait();
  //      })},

  //     {"smooth_surface_vertex_curve",  //
  //      "Smooth the current surface vertex curve by using the geodetic "
  //      "smoothing approach.",  //
  //      fun([](double tolerance, int laplace_iterations,
  //             double laplace_relaxation) {
  //        auto task = main_task_queue().result_from_push([=] {
  //          auto& viewer = main_viewer();
  //          viewer.tolerance = tolerance;
  //          viewer.laplace_iterations = laplace_iterations;
  //          viewer.laplace_relaxation = laplace_relaxation;
  //          viewer.compute_smooth_surface_mesh_curve();
  //        });
  //        task.wait();
  //      })},

  //     {"hyper_smooth_surface_vertex_curve",  //
  //      "Smooth the current surface vertex curve by using the hyper surface "
  //      "smoothing approach.",  //
  //      fun([](double lambda, int smoothing_passes) {
  //        auto task = main_task_queue().result_from_push([=] {
  //          auto& viewer = main_viewer();
  //          viewer.hyper_lambda = lambda;
  //          viewer.hyper_smoothing_passes = smoothing_passes;
  //          viewer.compute_hyper_surface_smoothing();
  //        });
  //        task.wait();
  //      })},

  //     {"set_wireframe",                       //
  //      "Turn on/off wireframe for shading.",  //
  //      fun([](bool value) {
  //        auto task = main_task_queue().result_from_push(
  //            [=] { main_viewer().set_wireframe(value); });
  //        task.wait();
  //      })},

  //     {"use_face_normal",                        //
  //      "Turn on/off face normals for shading.",  //
  //      fun([](bool value) {
  //        auto task = main_task_queue().result_from_push(
  //            [=] { main_viewer().use_face_normal(value); });
  //        task.wait();
  //      })},
  // };

  // // Add all module functions to the current lua thread.
  // //
  // for (const auto& [name, help, data] : functions) add(data, name);
}

}  // namespace ensketch::sandbox
