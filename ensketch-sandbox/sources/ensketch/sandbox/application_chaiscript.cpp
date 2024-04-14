#include <ensketch/sandbox/application.hpp>
//
#include <ensketch/sandbox/chaiscript.hpp>

namespace ensketch::sandbox {

namespace {

void chaiscript_init();

}  // namespace

void application::chaiscript_run() {
  chaiscript_thread = jthread{[this] {
    chaiscript_init();
    while (running) {
      while (running && chaiscript_tasks.process())
        ;
      // this_thread::yield();
      this_thread::sleep_for(100ms);
    }
  }};
}

void application::chaiscript_eval(const string& code) {
  chaiscript_tasks.push([code] {
    auto error = chaiscript::eval(code);
    if (!error) return;
    app().error(format("ChaiScript String Eval: {}", error->what()));
  });
}

void application::chaiscript_eval(const filesystem::path& path) {
  chaiscript_tasks.push([this, path] {
    const auto abs_path = absolute(path);
    lookup_path = abs_path.parent_path();
    auto error = chaiscript::eval(abs_path);
    lookup_path = filesystem::path{};
    if (!error) return;
    app().error(format("ChaiScript File Eval: {}", error->what()));
  });
}

namespace {

void chaiscript_init() {
  using namespace chaiscript;

  // Construct a static vector of all ChaiScript functions.
  // It needs to be static as the help function will need to refer to it.
  //
  static vector<tuple<string, string, function_type>> functions{
      {"help",                        //
       "Print available functions.",  //
       fun([&] {
         stringstream out{};
         out << "Available Functions:\n";
         for (const auto& [name, help, data] : functions)
           out << '\t' << name << '\n' << "\t\t" << help << "\n\n";
         app().info(out.str());
       })},

      {"quit",                   //
       "Quit the application.",  //
       fun([] { app().quit(); })},

      {"open_viewer",                              //
       "Open the viewer with an OpenGL context.",  //
       fun([](int width, int height) { app().open_viewer(width, height); })},

      {"close_viewer",                          //
       "Close the viewer and OpenGL context.",  //
       fun([] { app().close_viewer(); })},

      {"screenshot",                                         //
       "Stores the viewer's current framebuffer as image.",  //
       fun([](const string& path) {
         auto task = app().tasks.result_from_push(
             [path] { app().viewer.store_image_from_view(path); });
         task.wait();
       })},

      {"load_surface",                    //
       "Load a surface mesh from file.",  //
       fun([](const string& path) {
         auto task = app().tasks.result_from_push(
             [path] { app().viewer.load_surface(path); });
         task.wait();
       })},

      {"save_perspective",                        //
       "Save camera perspective to given file.",  //
       fun([](const string& path) { app().viewer.save_perspective(path); })},

      {"load_perspective",                          //
       "Load camera perspective from given file.",  //
       fun([](const string& path) {
         auto task = app().tasks.result_from_push(
             [path] { app().viewer.load_perspective(path); });
         task.wait();
       })},

      {"save_surface_vertex_curve",                         //
       "Save current surface vertex curve to given file.",  //
       fun([](const string& path) {
         app().viewer.save_surface_vertex_curve(path);
       })},

      {"load_surface_vertex_curve",                   //
       "Load surface vertex curve from given file.",  //
       fun([](const string& path) {
         auto task = app().tasks.result_from_push(
             [path] { app().viewer.load_surface_vertex_curve(path); });
         task.wait();
       })},

      {"smooth_surface_vertex_curve",  //
       "Smooth the current surface vertex curve by using the geodetic "
       "smoothing approach.",  //
       fun([](double tolerance, int laplace_iterations,
              double laplace_relaxation) {
         auto task = app().tasks.result_from_push([=] {
           auto& viewer = app().viewer;
           viewer.tolerance = tolerance;
           viewer.laplace_iterations = laplace_iterations;
           viewer.laplace_relaxation = laplace_relaxation;
           viewer.compute_smooth_surface_mesh_curve();
         });
         task.wait();
       })},

      {"hyper_smooth_surface_vertex_curve",  //
       "Smooth the current surface vertex curve by using the hyper surface "
       "smoothing approach.",  //
       fun([](double lambda, int smoothing_passes) {
         auto task = app().tasks.result_from_push([=] {
           auto& viewer = app().viewer;
           viewer.hyper_lambda = lambda;
           viewer.hyper_smoothing_passes = smoothing_passes;
           viewer.compute_hyper_surface_smoothing();
         });
         task.wait();
       })},

      {"set_wireframe",                       //
       "Turn on/off wireframe for shading.",  //
       fun([](bool value) {
         auto task = app().tasks.result_from_push(
             [=] { app().viewer.set_wireframe(value); });
         task.wait();
       })},

      {"use_face_normal",                        //
       "Turn on/off face normals for shading.",  //
       fun([](bool value) {
         auto task = app().tasks.result_from_push(
             [=] { app().viewer.use_face_normal(value); });
         task.wait();
       })},
  };

  // Add all module functions to the current ChaiScript thread.
  //
  for (const auto& [name, help, data] : functions) add(data, name);
}

}  // namespace

}  // namespace ensketch::sandbox
