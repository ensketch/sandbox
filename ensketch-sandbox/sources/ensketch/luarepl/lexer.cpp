#include "lexer.hpp"

namespace ensketch::luarepl::lexer {

auto scan(czstring text) -> context {
  context ctx{};
  auto it = iterator{text};
  while (*it) {
    if (match(it, is_whitespace)) continue;
    if (scan_for_comment(it, ctx)) continue;
    if (scan_for_identifier_or_keyword(it, ctx)) continue;
    if (scan_for_string(it, ctx)) continue;
    if (scan_for_numeral(it, ctx)) continue;
    if (scan_for_other_tokens(it, ctx)) continue;
    throw std::runtime_error(
        "Failed to scan given string. Unexpected codepoint.");
  }
  return ctx;
}

bool scan_for_comment(iterator& next, context& ctx) {
  const auto base = next;
  if (!match<"--">(next)) return false;

  ctx.comments.push_back(
      {.base = base.pointer(), .prefix_end = next.pointer()});
  auto& entry = ctx.comments.back();

  if (match_open_long_bracket(next)) {
    entry.open_end = next.pointer();
    const auto level = entry.open_bracket().size() - 2;
    for (; *next; ++next) {
      const auto tmp = next;
      if (match_close_long_bracket(next, level)) {
        entry.text_end = tmp.pointer();
        entry.close_end = next.pointer();
        return true;
      }
    }
    entry.text_end = next.pointer();
    entry.close_end = next.pointer();
    ctx.error(unfinished_long_comment{});
    return true;
  }

  entry.open_end = next.pointer();
  auto tmp = next;
  for (; *next; ++next) {
    tmp = next;
    if (match_end_of_line(next)) break;
  }
  entry.text_end = tmp.pointer();
  entry.close_end = tmp.pointer();
  return true;
}

bool scan_for_identifier_or_keyword(iterator& next, context& ctx) {
  const auto base = next;
  if (!match_identifier(next)) return false;
  string_view name{base.pointer(), next.pointer()};

  const auto is_keyword =
      for_each_until(keyword_types{}, [name, &ctx]<typename type> {
        if (name != type::string()) return false;
        ctx.tokens.emplace_back(type{name});
        return true;
      });

  if (!is_keyword) ctx.tokens.emplace_back(identifier{name});

  return true;
}

bool scan_for_other_tokens(iterator& next, context& ctx) {
  return traverse(meta::radix_tree_from(operators_and_punctuation{}),
                  next.pointer(), [&ctx, &next]<auto str>(auto tail) {
                    ctx.tokens.push_back(token{
                        terminal<str>{string_view{next.pointer(), tail}}});
                    next.ptr = tail;
                  });
}

bool scan_for_string(iterator& next, context& ctx) {
  const auto base = next;
  if (!match_string(next)) return false;
  ctx.tokens.emplace_back(
      string_literal{string_view{base.pointer(), next.pointer()}});
  return true;
}

bool scan_for_numeral(iterator& next, context& ctx) {
  const auto base = next;
  if (!match_numeral(next)) return false;
  ctx.tokens.emplace_back(numeral{string_view{base.pointer(), next.pointer()}});
  return true;
}

}  // namespace ensketch::luarepl::lexer
