#include <ensketch/sandbox/application.hpp>

using namespace ensketch::sandbox;

int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i)
    app().chaiscript_eval(filesystem::path(argv[i]));
  app().run();
}
