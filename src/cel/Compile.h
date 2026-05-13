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
// ─────────────────────────────────────────────

#include <cstddef>
#include <string>
#include <string_view>

#include "Expression.h"

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

    size_t i = 0;
    while (i < source.size() && isSpace(source[i])) ++i;

    if (i == source.size()) {
        // Empty or pure-whitespace source — leave Expression in its
        // default always-true state.
        return true;
    }

    // Parser not yet implemented for non-empty source. Report the
    // position of the first non-whitespace byte so callers see the gap.
    err.position = i;
    err.message  = "parser not yet implemented";
    return false;
}

} // namespace cel
} // namespace seam
