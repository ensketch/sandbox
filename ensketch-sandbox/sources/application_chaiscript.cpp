#include <application.hpp>
//
#include <chaiscript/chaiscript.hpp>

using namespace chaiscript;

// Do not export 'object'.
namespace {
struct object {
  string name;
  string description;
  Boxed_Value data;
};
}  // namespace

struct application::impl {
  ChaiScript chai{};
  vector<object> objects{};
};

application::~application() noexcept {
  if (pimpl) delete pimpl;
}

void application::eval_chaiscript(const filesystem::path& script) try {
  pimpl->chai.eval_file(script);
} catch (chaiscript::exception::eval_error& e) {
  console.log(format("ERROR: ChaiScript File Eval {}\n", e.what()));
}

void application::eval_chaiscript(const string& code) try {
  pimpl->chai.eval(code);
} catch (chaiscript::exception::eval_error& e) {
  console.log(format("ERROR: ChaiScript String Eval {}\n", e.what()));
}

void application::init_chaiscript() {
  pimpl = new impl{};

  pimpl->objects.emplace_back("viewer_open", "Open the viewer.",
                              var(fun([this](int width, int height) {
                                viewer.open(width, height);
                                viewer.set_vsync();
                                timer.set_syncing(false);
                              })));

  pimpl->objects.emplace_back("viewer_close", "Close the viewer.",
                              var(fun([this](int width, int height) {
                                viewer.close();
                                timer.set_syncing();
                              })));

  pimpl->objects.emplace_back(
      "set_opengl_version", "Set the OpenGL version used for the viewer.",
      var(fun([this](int major, int minor) {
        ensketch::sandbox::viewer::opengl_context_settings.majorVersion = major;
        ensketch::sandbox::viewer::opengl_context_settings.minorVersion = minor;
      })));

  pimpl->objects.emplace_back(
      "help", "Print available functions.", var(fun([this] {
        cout << "Available Functions:\n";
        for (auto& x : pimpl->objects)
          cout << '\t' << x.name << '\n' << "\t\t" << x.description << "\n\n";
        cout << flush;
      })));

  for (auto& x : pimpl->objects) pimpl->chai.add(x.data, x.name);
}
