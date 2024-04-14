#pragma once
#include <ensketch/sandbox/utility.hpp>
//
// #include <chaiscript/chaiscript_basic.hpp>
#include <chaiscript/chaiscript_defines.hpp>
#include <chaiscript/dispatchkit/boxed_number.hpp>
#include <chaiscript/dispatchkit/dispatchkit.hpp>
#include <chaiscript/dispatchkit/dynamic_object.hpp>
#include <chaiscript/dispatchkit/function_call.hpp>
#include <chaiscript/language/chaiscript_eval.hpp>

namespace ensketch::sandbox::chaiscript {

using namespace ::chaiscript;

using error_type = exception::eval_error;
using value_type = Boxed_Value;
using function_type = Proxy_Function;

auto eval(const filesystem::path& script) -> optional<error_type>;

auto eval(const string& code) -> optional<error_type>;

void add(value_type value, const string& name);

void add(function_type function, const string& name);

inline void add(auto&& f, const string& name) {
  add(fun(forward<decltype(f)>(f)), name);
}

}  // namespace ensketch::sandbox::chaiscript
