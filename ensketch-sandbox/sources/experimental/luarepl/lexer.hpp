#pragma once
#include "token.hpp"

namespace ensketch::luarepl::lexer {

using codepoint = uint32_t;

constexpr bool is_whitespace(codepoint c) noexcept {
  return (c == codepoint(' ')) ||
         ((codepoint('\t') <= c) && (c <= codepoint('\r')));
}

constexpr bool is_digit(codepoint c) noexcept {
  return (codepoint('0') <= c) && (c <= codepoint('9'));
}

constexpr bool is_lowercase_letter(codepoint c) noexcept {
  return (codepoint('a') <= c) && (c <= codepoint('z'));
}

constexpr bool is_uppercase_letter(codepoint c) noexcept {
  return (codepoint('A') <= c) && (c <= codepoint('Z'));
}

constexpr auto lowercase_letter(codepoint c) noexcept -> codepoint {
  return c | 0b0010'0000;
}

constexpr auto uppercase_letter(codepoint c) noexcept -> codepoint {
  return c & 0b1101'1111;
}

constexpr bool is_letter(codepoint c) noexcept {
  return is_lowercase_letter(lowercase_letter(c));
}

constexpr bool is_identifier_start(codepoint c) noexcept {
  return (c == codepoint('_')) || is_letter(c);
}

constexpr bool is_identifier_tail(codepoint c) noexcept {
  return is_identifier_start(c) || is_digit(c);
}

struct iterator {
  constexpr iterator() noexcept = default;
  constexpr iterator(czstring str) noexcept : ptr{str} { assert(ptr); }

  friend constexpr auto operator-(iterator i, iterator j) noexcept {
    return i.ptr - j.ptr;
  }

  constexpr auto operator++() noexcept -> iterator& {
    assert(*ptr);
    ++ptr;
    return *this;
  }

  constexpr auto operator*() const noexcept -> codepoint { return *ptr; }

  constexpr auto pointer() const noexcept -> czstring { return ptr; }

  czstring ptr = "";
};

template <codepoint c>
constexpr bool match(iterator& it) noexcept {
  if (*it != c) return false;
  ++it;
  return true;
}

constexpr bool match(iterator& it, codepoint c) noexcept {
  if (*it != c) return false;
  ++it;
  return true;
}

constexpr bool match(iterator& it, auto&& f) noexcept
  requires requires(codepoint c) {
    { std::invoke(std::forward<decltype(f)>(f), c) } -> std::same_as<bool>;
  }
{
  if (!std::invoke(std::forward<decltype(f)>(f), *it)) return false;
  ++it;
  return true;
}

namespace detail {
template <meta::string str>
constexpr bool match(iterator& it) noexcept {
  if constexpr (str.empty())
    return true;
  else {
    if (match<str[0]>(it)) return detail::match<tail<1>(str)>(it);
    return false;
  }
}
}  // namespace detail

template <meta::string str>
constexpr bool match(iterator& it) noexcept {
  auto tmp = it;
  if (!detail::match<str>(tmp)) return false;
  it = tmp;
  return true;
}

constexpr bool match(iterator& it, string_view str) noexcept {
  auto tmp = it;
  for (auto c : str)
    if (!match(tmp, c)) return false;
  it = tmp;
  return true;
}

constexpr bool match_end_of_line(iterator& it) noexcept {
  if (match<'\n'>(it)) {
    match<'\r'>(it);
    return true;
  }
  if (match<'\r'>(it)) {
    match<'\n'>(it);
    return true;
  }
  return false;
}

constexpr bool match_open_long_bracket(iterator& it) noexcept {
  auto tmp = it;
  if (match<'['>(tmp)) {
    while (match<'='>(tmp));
    if (match<'['>(tmp)) {
      it = tmp;
      return true;
    }
  }
  return false;
}

constexpr bool match_close_long_bracket(iterator& it) noexcept {
  auto tmp = it;
  if (match<']'>(tmp)) {
    while (match<'='>(tmp));
    if (match<']'>(tmp)) {
      it = tmp;
      return true;
    }
  }
  return false;
}

constexpr bool match_close_long_bracket(iterator& it, size_t level) noexcept {
  auto tmp = it;
  if (match<']'>(tmp)) {
    for (; level > 0; --level)
      if (!match<'='>(tmp)) return false;
    if (!match<']'>(tmp)) return false;
    it = tmp;
    return true;
  }
  return false;
}

constexpr bool match_identifier(iterator& it) noexcept {
  if (match(it, is_identifier_start)) {
    while (match(it, is_identifier_tail));
    return true;
  }
  return false;
}

constexpr bool match_string(iterator& it) noexcept {
  if (match(it, '"')) {
    for (; *it; ++it)
      if (match(it, '"')) return true;
    return true;
  }
  return false;
}

constexpr bool match_numeral(iterator& it) noexcept {
  if (match(it, is_digit)) {
    while (match(it, is_digit));
    return true;
  }
  return false;
}

struct error_type_base {};
struct unfinished_long_comment : error_type_base {
  static auto what() noexcept { return "Unfinished long comment near EOF."sv; }
};
using error_entry_base = std::variant<unfinished_long_comment>;
struct error_entry : error_entry_base {
  using base = error_entry_base;
};

struct comment_entry {
  auto all() const noexcept -> string_view { return {base, close_end}; }
  auto prefix() const noexcept -> string_view { return {base, prefix_end}; }
  auto open_bracket() const noexcept -> string_view {
    return {prefix_end, open_end};
  }
  auto text() const noexcept -> string_view { return {open_end, text_end}; }
  auto close_bracket() const noexcept -> string_view {
    return {text_end, close_end};
  }

  czstring base{};
  czstring prefix_end{};
  czstring open_end{};
  czstring text_end{};
  czstring close_end{};
};

struct context {
  context() noexcept = default;

  constexpr bool valid() const noexcept { return errors.empty(); }

  constexpr void error(auto&& err) {
    errors.emplace_back(std::forward<decltype(err)>(err));
  };

  std::vector<token> tokens{};
  std::vector<comment_entry> comments{};
  std::vector<error_entry> errors{};
};

auto scan(czstring text) -> context;
bool scan_for_comment(iterator& it, context& ctx);
bool scan_for_identifier_or_keyword(iterator& next, context& ctx);
bool scan_for_other_tokens(iterator& next, context& ctx);
bool scan_for_string(iterator& next, context& ctx);
bool scan_for_numeral(iterator& next, context& ctx);

}  // namespace ensketch::luarepl::lexer
