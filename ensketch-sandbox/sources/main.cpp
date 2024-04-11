#include <ensketch/sandbox/application.hpp>

using namespace ensketch::sandbox;

int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i)
    app().async_eval_chaiscript(filesystem::path(argv[i]));
  app().run();
}
