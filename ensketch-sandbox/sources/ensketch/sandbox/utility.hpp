#pragma once
#include <optional>
//
#include <ranges>
//
#include <memory>
//
#include <chrono>
#include <future>
#include <thread>
//
#include <ensketch/xstd/utility.hpp>
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

using namespace std;
using namespace xstd;

using namespace gl;
using namespace glm;

using clock = chrono::high_resolution_clock;
using duration = chrono::duration<float32>;

}  // namespace ensketch::sandbox
