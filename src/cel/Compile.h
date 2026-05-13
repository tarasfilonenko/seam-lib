#pragma once
// ─────────────────────────────────────────────
// seam::cel::compile
//
// Parses a SEAM-CEL source string into an Expression.
//
// Grammar (precedence low → high):
//   expr      := or_expr
//   or_expr   := and_expr ( "||" and_expr )*
//   and_expr  := not_expr ( "&&" not_expr )*
//   not_expr  := "!" not_expr | cmp_expr
//   cmp_expr  := primary ( ( "==" | "!=" | "<" | "<=" | ">" | ">=" | "in" ) primary )?
//   primary   := number | string | "true" | "false" | identifier | "(" expr ")"
//   identifier := bare_id | "@" bare_id
//   bare_id   := /[A-Za-z_][A-Za-z0-9_]*/
//   number    := /-? [0-9]+ ( "." [0-9]+ )?/
//   string    := /"([^"\\]|\\.)*"/
//
// Empty or pure-whitespace source → Expression::isAlwaysTrue() == true.
//
// Return value:
//   true   — `out` holds the compiled expression, `err` reset.
//   false  — `out` is reset to an empty Expression (always-true), `err`
//            populated with byte offset + message describing the failure.
//
// Caller policy (e.g. ModuleModel): on failure, treat the source as
// "always true" (fail-open) and log the error + source text. This avoids
// hiding parts of the UI because a module shipped a typo in caps.
//
// Implementation is added incrementally by grammar feature. Until each
// feature lands, sources that exercise it return a parse error with the
// position pointing at the unrecognised token.
//
// Current coverage:
//   step 1 ─ empty / pure-whitespace source
//   step 2 ─ boolean literals (true, false)
// ─────────────────────────────────────────────

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "Expression.h"
#include "detail/Node.h"

namespace seam {
namespace cel {

struct CompileError {
    size_t      position = 0;       // byte offset into source where error was detected
    std::string message;            // short reason, e.g. "expected operator"
};

inline bool compile(std::string_view source, Expression &out, CompileError &err) {
    out = Expression{};
    err = CompileError{};

    auto isSpace = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };
    auto isAlpha = [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
    };
    auto isAlnum = [&](char c) {
        return isAlpha(c) || (c >= '0' && c <= '9');
    };

    size_t pos = 0;
    while (pos < source.size() && isSpace(source[pos])) ++pos;
    if (pos == source.size()) {
        return true;
    }

    if (!isAlpha(source[pos])) {
        err.position = pos;
        err.message  = "expected identifier or literal";
        return false;
    }

    const size_t word_start = pos;
    while (pos < source.size() && isAlnum(source[pos])) ++pos;
    std::string_view word = source.substr(word_start, pos - word_start);

    auto node = std::make_unique<detail::Node>();
    if (word == "true") {
        node->kind     = detail::Node::Kind::LitBool;
        node->bool_val = true;
    } else if (word == "false") {
        node->kind     = detail::Node::Kind::LitBool;
        node->bool_val = false;
    } else {
        err.position = word_start;
        err.message  = "unknown identifier";
        return false;
    }

    while (pos < source.size() && isSpace(source[pos])) ++pos;
    if (pos != source.size()) {
        err.position = pos;
        err.message  = "unexpected trailing content";
        return false;
    }

    out._root = std::move(node);
    return true;
}

} // namespace cel
} // namespace seam
