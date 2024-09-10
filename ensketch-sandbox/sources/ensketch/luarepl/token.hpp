#pragma once
#include "defaults.hpp"

namespace ensketch::luarepl {

struct token_type_base {
  friend constexpr auto operator<=>(const token_type_base&,
                                    const token_type_base&) noexcept = default;

  constexpr auto source() const noexcept { return src; }
  string_view src{};
};

struct identifier : token_type_base {};

struct numeral : token_type_base {};

struct string_literal : token_type_base {};

template <meta::string str>
struct terminal : token_type_base {
  static constexpr auto string() noexcept -> czstring {
    // return {str.begin(), str.end()};
    return str.data();
  }
};

using value_keywords = meta::name_list<"false", "nil", "true">;
using operator_keywords = meta::name_list<"and", "not", "or">;
using control_keywords = meta::name_list<"break",
                                         "do",
                                         "else",
                                         "elseif",
                                         "end",
                                         "for",
                                         "function",
                                         "goto",
                                         "if",
                                         "in",
                                         "local",
                                         "repeat",
                                         "return",
                                         "then",
                                         "until",
                                         "while">;
using keywords =
    decltype(value_keywords{} + operator_keywords{} + control_keywords{});

using operators_and_punctuation = meta::name_list<  //
    "+",
    "-",
    "*",
    "/",
    "%",
    "^",
    "#",
    "&",
    "~",
    "|",
    "<<",
    ">>",
    "//",
    "==",
    "~=",
    "<=",
    ">=",
    "<",
    ">",
    "=",
    "(",
    ")",
    "{",
    "}",
    "[",
    "]",
    "::",
    ";",
    ":",
    ",",
    ".",
    "..",
    "...">;

namespace detail {
template <typename type>
struct is_control_keyword : std::false_type {};
template <meta::string str>
struct is_control_keyword<terminal<str>> {
  static constexpr bool value = contained<str>(control_keywords{});
};
template <typename type>
struct is_operator_keyword : std::false_type {};
template <meta::string str>
struct is_operator_keyword<terminal<str>> {
  static constexpr bool value = contained<str>(operator_keywords{});
};
template <typename type>
struct is_value_keyword : std::false_type {};
template <meta::string str>
struct is_value_keyword<terminal<str>> {
  static constexpr bool value = contained<str>(value_keywords{});
};
template <typename type>
struct is_operator_or_punctuation : std::false_type {};
template <meta::string str>
struct is_operator_or_punctuation<terminal<str>> {
  static constexpr bool value = contained<str>(operators_and_punctuation{});
};
}  // namespace detail

template <typename type>
concept control_keyword = detail::is_control_keyword<type>::value;
template <typename type>
concept operator_keyword = detail::is_operator_keyword<type>::value;
template <typename type>
concept value_keyword = detail::is_value_keyword<type>::value;
template <typename type>
concept operator_or_punctuation =
    detail::is_operator_or_punctuation<type>::value;

// Use transform of `value_list` to `a` with predicate.
template <meta::string... str>
consteval auto terminal_list_from(meta::name_list<str...>) {
  return meta::type_list<terminal<str>...>{};
}

using keyword_types = decltype(terminal_list_from(keywords{}));
using operators_and_punctuation_types =
    decltype(terminal_list_from(operators_and_punctuation{}));

using terminals = decltype(keyword_types{} + operators_and_punctuation_types{});

using token_types =
    decltype(meta::type_list<identifier, numeral, string_literal>{} +
             terminals{});

template <typename... types>
consteval auto variant_from(meta::type_list<types...>)
    -> std::variant<types...>;

using token_base = decltype(variant_from(token_types{}));
struct token : token_base {
  using token_base::token_base;
};

}  // namespace ensketch::luarepl
