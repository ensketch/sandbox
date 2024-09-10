#pragma once
#include <optional>
//
#include <queue>
#include <ranges>
//
#include <memory>
//
#include <chrono>
#include <future>
#include <thread>
//
#include <ensketch/xstd/xstd.hpp>
//
#include <ensketch/opengl/opengl.hpp>

#define ENSKETCH_SANDBOX_ADD_DEFAULT_CONSTRUCTOR_EXTENSION(TYPE)       \
  constexpr auto TYPE##_from(auto&&... args) noexcept(                 \
      noexcept(TYPE(std::forward<decltype(args)>(args)...)))           \
    requires requires { TYPE(std::forward<decltype(args)>(args)...); } \
  {                                                                    \
    return TYPE(std::forward<decltype(args)>(args)...);                \
  }

namespace ensketch::sandbox {

// using namespace std;
namespace filesystem = std::filesystem;
using std::optional;
using std::scoped_lock;
using std::string;
using std::string_view;

using namespace xstd;

using namespace gl;
using namespace glm;

using clock = std::chrono::high_resolution_clock;
using duration = std::chrono::duration<float32>;

}  // namespace ensketch::sandbox
