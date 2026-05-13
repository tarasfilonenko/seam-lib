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
//   step 3 ─ number literals (int + decimal, optional leading '-')
//            string literals with \" \\ \n \t \r escapes
//   step 4 ─ identifiers (bare + '@'-prefixed) + references tracking
// ─────────────────────────────────────────────

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
    auto isDigit = [](char c) {
        return c >= '0' && c <= '9';
    };
    auto isAlnum = [&](char c) {
        return isAlpha(c) || isDigit(c);
    };

    // References collected during parse; moved into out._references on
    // success so partially-parsed expressions don't leak refs on error.
    std::vector<std::string> refs;
    auto addRef = [&](std::string name) {
        if (std::find(refs.begin(), refs.end(), name) == refs.end()) {
            refs.push_back(std::move(name));
        }
    };

    size_t pos = 0;
    while (pos < source.size() && isSpace(source[pos])) ++pos;
    if (pos == source.size()) {
        return true;
    }

    auto node = std::make_unique<detail::Node>();
    const char first = source[pos];
    const bool startsNumber =
        isDigit(first) ||
        (first == '-' && pos + 1 < source.size() && isDigit(source[pos + 1]));

    if (isAlpha(first)) {
        const size_t word_start = pos;
        while (pos < source.size() && isAlnum(source[pos])) ++pos;
        std::string_view word = source.substr(word_start, pos - word_start);
        if (word == "true") {
            node->kind     = detail::Node::Kind::LitBool;
            node->bool_val = true;
        } else if (word == "false") {
            node->kind     = detail::Node::Kind::LitBool;
            node->bool_val = false;
        } else {
            node->kind    = detail::Node::Kind::Identifier;
            node->str_val = std::string(word);
            addRef(node->str_val);
        }
    }
    else if (first == '@') {
        const size_t at_pos = pos;
        ++pos;
        // Need a bare_id start (letter or '_') after '@'.
        if (pos >= source.size() || !isAlpha(source[pos])) {
            err.position = at_pos;
            err.message  = "expected identifier after '@'";
            return false;
        }
        while (pos < source.size() && isAlnum(source[pos])) ++pos;
        node->kind    = detail::Node::Kind::Identifier;
        node->str_val = std::string(source.substr(at_pos, pos - at_pos));   // includes '@'
        addRef(node->str_val);
    }
    else if (startsNumber) {
        const size_t num_start = pos;
        if (source[pos] == '-') ++pos;
        while (pos < source.size() && isDigit(source[pos])) ++pos;
        if (pos < source.size() && source[pos] == '.') {
            const size_t dot_pos    = pos;
            ++pos;
            const size_t frac_start = pos;
            while (pos < source.size() && isDigit(source[pos])) ++pos;
            if (pos == frac_start) {
                err.position = dot_pos;
                err.message  = "expected digits after '.'";
                return false;
            }
        }

        // Copy the lexed slice into a NUL-terminated buffer for strtod.
        // The lexer has already validated the format, so strtod consuming
        // the entire buffer is just defensive — anything else means a
        // lexer/parser drift.
        std::string buf(source.substr(num_start, pos - num_start));
        char  *end = nullptr;
        double v   = std::strtod(buf.c_str(), &end);
        if (end != buf.c_str() + buf.size()) {
            err.position = num_start;
            err.message  = "invalid number";
            return false;
        }
        node->kind    = detail::Node::Kind::LitNumber;
        node->num_val = v;
    }
    else if (first == '"') {
        const size_t str_start = pos;
        ++pos;                              // skip opening quote
        std::string  decoded;
        bool         closed = false;

        while (pos < source.size()) {
            const char ch = source[pos];
            if (ch == '"') {
                closed = true;
                ++pos;
                break;
            }
            if (ch == '\\') {
                ++pos;
                if (pos >= source.size()) break;   // → unterminated below
                const char esc = source[pos];
                ++pos;
                switch (esc) {
                    case '"':  decoded.push_back('"');  break;
                    case '\\': decoded.push_back('\\'); break;
                    case 'n':  decoded.push_back('\n'); break;
                    case 't':  decoded.push_back('\t'); break;
                    case 'r':  decoded.push_back('\r'); break;
                    default:
                        err.position = pos - 1;        // offending escape char
                        err.message  = "invalid escape";
                        return false;
                }
                continue;
            }
            decoded.push_back(ch);
            ++pos;
        }

        if (!closed) {
            err.position = str_start;
            err.message  = "unterminated string";
            return false;
        }

        node->kind    = detail::Node::Kind::LitString;
        node->str_val = std::move(decoded);
    }
    else {
        err.position = pos;
        err.message  = "expected identifier or literal";
        return false;
    }

    while (pos < source.size() && isSpace(source[pos])) ++pos;
    if (pos != source.size()) {
        err.position = pos;
        err.message  = "unexpected trailing content";
        return false;
    }

    out._root       = std::move(node);
    out._references = std::move(refs);
    return true;
}

} // namespace cel
} // namespace seam
