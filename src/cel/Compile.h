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
//   step 5 ─ parenthesised primaries + recursive parser structure
//            (parseExpr / parsePrimary split via detail::ParseState)
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

namespace detail {

// Mutable parser state carried through parseExpr / parsePrimary and any
// helpers. Holds the source view, the cursor, and pointers to the
// caller-owned `refs` vector and `err` struct. Pointer semantics keep
// the state struct cheap to pass around without aliasing the
// Expression's own members during partial parses.
struct ParseState {
    std::string_view          source;
    size_t                    pos  = 0;
    std::vector<std::string> *refs = nullptr;
    CompileError             *err  = nullptr;

    bool atEnd() const noexcept { return pos >= source.size(); }
    char peek()  const noexcept { return atEnd() ? '\0' : source[pos]; }

    static bool isSpace(char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
    static bool isAlpha(char c) noexcept { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
    static bool isDigit(char c) noexcept { return c >= '0' && c <= '9'; }
    static bool isAlnum(char c) noexcept { return isAlpha(c) || isDigit(c); }

    void skipWhitespace() {
        while (!atEnd() && isSpace(source[pos])) ++pos;
    }

    void fail(size_t at, const char *msg) {
        err->position = at;
        err->message  = msg;
    }

    void addRef(std::string name) {
        if (std::find(refs->begin(), refs->end(), name) == refs->end()) {
            refs->push_back(std::move(name));
        }
    }
};

// Forward decl — parsePrimary needs parseExpr for the paren case.
inline std::unique_ptr<Node> parseExpr(ParseState &p);

inline std::unique_ptr<Node> parsePrimary(ParseState &p) {
    const char first = p.peek();

    // ── (expr) ─────────────────────────────────
    if (first == '(') {
        const size_t open_pos = p.pos;
        ++p.pos;
        p.skipWhitespace();
        auto inner = parseExpr(p);
        if (!inner) return nullptr;
        p.skipWhitespace();
        if (p.atEnd() || p.peek() != ')') {
            if (p.atEnd()) p.fail(open_pos, "unclosed '('");
            else           p.fail(p.pos,    "expected ')'");
            return nullptr;
        }
        ++p.pos;
        return inner;
    }

    auto node = std::make_unique<Node>();
    const bool startsNumber =
        ParseState::isDigit(first) ||
        (first == '-' && p.pos + 1 < p.source.size() && ParseState::isDigit(p.source[p.pos + 1]));

    // ── alpha-word: keyword (true/false) or bare identifier ────
    if (ParseState::isAlpha(first)) {
        const size_t word_start = p.pos;
        while (!p.atEnd() && ParseState::isAlnum(p.peek())) ++p.pos;
        std::string_view word = p.source.substr(word_start, p.pos - word_start);
        if (word == "true") {
            node->kind     = Node::Kind::LitBool;
            node->bool_val = true;
        } else if (word == "false") {
            node->kind     = Node::Kind::LitBool;
            node->bool_val = false;
        } else {
            node->kind    = Node::Kind::Identifier;
            node->str_val = std::string(word);
            p.addRef(node->str_val);
        }
        return node;
    }

    // ── @bare_id ───────────────────────────────
    if (first == '@') {
        const size_t at_pos = p.pos;
        ++p.pos;
        if (p.atEnd() || !ParseState::isAlpha(p.peek())) {
            p.fail(at_pos, "expected identifier after '@'");
            return nullptr;
        }
        while (!p.atEnd() && ParseState::isAlnum(p.peek())) ++p.pos;
        node->kind    = Node::Kind::Identifier;
        node->str_val = std::string(p.source.substr(at_pos, p.pos - at_pos));   // includes '@'
        p.addRef(node->str_val);
        return node;
    }

    // ── number ─────────────────────────────────
    if (startsNumber) {
        const size_t num_start = p.pos;
        if (p.peek() == '-') ++p.pos;
        while (!p.atEnd() && ParseState::isDigit(p.peek())) ++p.pos;
        if (!p.atEnd() && p.peek() == '.') {
            const size_t dot_pos    = p.pos;
            ++p.pos;
            const size_t frac_start = p.pos;
            while (!p.atEnd() && ParseState::isDigit(p.peek())) ++p.pos;
            if (p.pos == frac_start) {
                p.fail(dot_pos, "expected digits after '.'");
                return nullptr;
            }
        }

        // Copy the lexed slice into a NUL-terminated buffer for strtod.
        // The lexer has already validated the format, so strtod consuming
        // the entire buffer is just defensive — anything else means a
        // lexer/parser drift.
        std::string buf(p.source.substr(num_start, p.pos - num_start));
        char  *end = nullptr;
        double v   = std::strtod(buf.c_str(), &end);
        if (end != buf.c_str() + buf.size()) {
            p.fail(num_start, "invalid number");
            return nullptr;
        }
        node->kind    = Node::Kind::LitNumber;
        node->num_val = v;
        return node;
    }

    // ── string ─────────────────────────────────
    if (first == '"') {
        const size_t str_start = p.pos;
        ++p.pos;                              // skip opening quote
        std::string  decoded;
        bool         closed = false;

        while (!p.atEnd()) {
            const char ch = p.peek();
            if (ch == '"') {
                closed = true;
                ++p.pos;
                break;
            }
            if (ch == '\\') {
                ++p.pos;
                if (p.atEnd()) break;          // → unterminated below
                const char esc = p.peek();
                ++p.pos;
                switch (esc) {
                    case '"':  decoded.push_back('"');  break;
                    case '\\': decoded.push_back('\\'); break;
                    case 'n':  decoded.push_back('\n'); break;
                    case 't':  decoded.push_back('\t'); break;
                    case 'r':  decoded.push_back('\r'); break;
                    default:
                        p.fail(p.pos - 1, "invalid escape");   // offending escape char
                        return nullptr;
                }
                continue;
            }
            decoded.push_back(ch);
            ++p.pos;
        }

        if (!closed) {
            p.fail(str_start, "unterminated string");
            return nullptr;
        }

        node->kind    = Node::Kind::LitString;
        node->str_val = std::move(decoded);
        return node;
    }

    // ── nothing matched (also covers EOF: peek() returns '\0') ──
    p.fail(p.pos, "expected identifier or literal");
    return nullptr;
}

// Top-level expression. For now it's a thin wrapper around parsePrimary;
// later steps (6+) will dispatch through the precedence chain
// (or_expr → and_expr → not_expr → cmp_expr → primary).
inline std::unique_ptr<Node> parseExpr(ParseState &p) {
    p.skipWhitespace();
    return parsePrimary(p);
}

} // namespace detail

inline bool compile(std::string_view source, Expression &out, CompileError &err) {
    out = Expression{};
    err = CompileError{};

    std::vector<std::string> refs;
    detail::ParseState p{ source, 0, &refs, &err };

    p.skipWhitespace();
    if (p.atEnd()) {
        return true;                            // empty / whitespace
    }

    auto node = detail::parseExpr(p);
    if (!node) {
        return false;                           // err already populated
    }

    p.skipWhitespace();
    if (!p.atEnd()) {
        err.position = p.pos;
        err.message  = "unexpected trailing content";
        return false;
    }

    out._root       = std::move(node);
    out._references = std::move(refs);
    return true;
}

} // namespace cel
} // namespace seam
