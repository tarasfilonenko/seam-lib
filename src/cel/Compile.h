#pragma once
// ─────────────────────────────────────────────
// seam::cel::compile
//
// Parses a SEAM-CEL source string into an Expression.
//
// Grammar (precedence low → high):
//   expr      := or_expr
//   or_expr   := and_expr ( "||" and_expr )*
//   and_expr  := cmp_expr ( "&&" cmp_expr )*
//   cmp_expr  := not_expr ( ( "==" | "!=" | "<" | "<=" | ">" | ">=" | "in" ) not_expr )?
//   not_expr  := "!" not_expr | primary
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
//            (detail::ParseState holds per-production methods)
//   step 6 ─ unary '!' (right-associative; type-strict on bool)
//   step 7 ─ comparisons: == != < <= > >= (non-chainable, type-strict)
//            Precedence: cmp_expr sits ABOVE not_expr, so
//            `!a == b` parses as `(!a) == b` — matching C/CEL.
//   step 8 ─ logical && and || (left-associative chains; three-valued
//            with error absorption: false absorbs in &&, true absorbs
//            in ||, anything else with a non-bool operand → Undefined).
//            && binds tighter than ||.
//   step 9 ─ membership 'in' (keyword operator at cmp_expr precedence;
//            string-only; rhs is space-tokenised).
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

// Binary-operator table entry. Used by parseOr / parseAnd / parseCmp
// via ParseState::tryMatchOp. `is_keyword == true` means the match
// also requires a word boundary after `text` (so `in` doesn't grab
// the start of `index`). Symbol operators set `is_keyword == false`.
struct Op {
    std::string_view text;
    Node::Kind       kind;
    bool             is_keyword;
};

// Operator tables — one per precedence rung. Order matters: longer
// ops first so dispatch returns on the most specific match (`<=`
// before `<`). Adding a new operator at any level is a one-line
// table entry.
inline constexpr Op kOrOps[]  = {
    { "||", Node::Kind::Or,  false },
};
inline constexpr Op kAndOps[] = {
    { "&&", Node::Kind::And, false },
};
inline constexpr Op kCmpOps[] = {
    { "==", Node::Kind::Eq,  false },
    { "!=", Node::Kind::Neq, false },
    { "<=", Node::Kind::Le,  false },
    { ">=", Node::Kind::Ge,  false },
    { "<",  Node::Kind::Lt,  false },
    { ">",  Node::Kind::Gt,  false },
    { "in", Node::Kind::In,  true  },
};

// Recursive-descent parser carrying source + cursor + outputs. Each
// grammar production is a method that returns a parsed Node (or
// nullptr on failure, with the failure recorded in *err). The cursor
// advances as productions consume input.
//
// Pointer semantics for `refs` and `err` keep the state struct cheap
// to copy/move and let the public compile() own the actual vector +
// CompileError without ParseState aliasing Expression's members
// during partial parses.
struct ParseState {
    std::string_view          source;
    size_t                    pos  = 0;
    std::vector<std::string> *refs = nullptr;
    CompileError             *err  = nullptr;

    // ── cursor helpers ────────────────────────

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

    // Look-ahead for multi-char tokens (e.g. "==", "!=", "<=").
    bool matches(std::string_view s) const noexcept {
        if (pos + s.size() > source.size()) return false;
        for (size_t i = 0; i < s.size(); ++i) {
            if (source[pos + i] != s[i]) return false;
        }
        return true;
    }

    // Like matches(), but only succeeds if the character immediately
    // after the keyword is NOT an identifier-continuation char. Used for
    // letter-based operators ('in') that would otherwise be confused
    // with identifiers like `index` or `inversion`. Symbol operators
    // (==, <=, &&, …) don't need this — their chars aren't valid in
    // identifiers, so plain matches() is unambiguous.
    bool matchKeyword(std::string_view kw) const noexcept {
        if (!matches(kw)) return false;
        const size_t after = pos + kw.size();
        if (after < source.size() && isAlnum(source[after])) return false;
        return true;
    }

    // Operator-table dispatch: scans `ops` for the first entry that
    // matches at the current cursor. Returns a pointer into the table
    // (caller advances pos by op->text.size()), or nullptr if none
    // match. Keeps every operator-level parser shaped the same way and
    // makes adding new operators a one-line table entry.
    //
    // Order within each table matters: longer ops MUST come before
    // their prefix-shorter versions (`<=` before `<`, `>=` before `>`)
    // because we return on first match.
    template <size_t N>
    const Op *tryMatchOp(const Op (&ops)[N]) const noexcept {
        for (const auto &op : ops) {
            const bool ok = op.is_keyword ? matchKeyword(op.text)
                                          : matches(op.text);
            if (ok) return &op;
        }
        return nullptr;
    }

    // ── grammar productions ───────────────────
    //
    // Each returns a Node on success or nullptr on failure (with err
    // populated). Defined out-of-line below for readability.

    std::unique_ptr<Node> parseExpr();
    std::unique_ptr<Node> parseOr();
    std::unique_ptr<Node> parseAnd();
    std::unique_ptr<Node> parseCmp();
    std::unique_ptr<Node> parseNot();
    std::unique_ptr<Node> parsePrimary();

    std::unique_ptr<Node> parseParen();
    std::unique_ptr<Node> parseWordOrKeyword();
    std::unique_ptr<Node> parseAtIdentifier();
    std::unique_ptr<Node> parseNumber();
    std::unique_ptr<Node> parseString();
};

// ── expr ──────────────────────────────────────
//
// Top-level entry. Routes through the precedence chain:
//   parseOr → parseAnd → parseCmp → parseNot → parsePrimary
inline std::unique_ptr<Node> ParseState::parseExpr() {
    skipWhitespace();
    return parseOr();
}

// ── or_expr ───────────────────────────────────
//
// or_expr := and_expr ( "||" and_expr )*
//
// Left-associative chain: `a || b || c` → Or(Or(a, b), c).
inline std::unique_ptr<Node> ParseState::parseOr() {
    auto lhs = parseAnd();
    if (!lhs) return nullptr;
    while (true) {
        skipWhitespace();
        const Op *op = tryMatchOp(kOrOps);
        if (!op) break;
        pos += op->text.size();
        skipWhitespace();
        auto rhs = parseAnd();
        if (!rhs) return nullptr;
        auto node  = std::make_unique<Node>();
        node->kind = op->kind;
        node->lhs  = std::move(lhs);
        node->rhs  = std::move(rhs);
        lhs        = std::move(node);
    }
    return lhs;
}

// ── and_expr ──────────────────────────────────
//
// and_expr := cmp_expr ( "&&" cmp_expr )*
//
// Left-associative; '&&' binds tighter than '||'.
inline std::unique_ptr<Node> ParseState::parseAnd() {
    auto lhs = parseCmp();
    if (!lhs) return nullptr;
    while (true) {
        skipWhitespace();
        const Op *op = tryMatchOp(kAndOps);
        if (!op) break;
        pos += op->text.size();
        skipWhitespace();
        auto rhs = parseCmp();
        if (!rhs) return nullptr;
        auto node  = std::make_unique<Node>();
        node->kind = op->kind;
        node->lhs  = std::move(lhs);
        node->rhs  = std::move(rhs);
        lhs        = std::move(node);
    }
    return lhs;
}

// ── cmp_expr ──────────────────────────────────
//
// cmp_expr := not_expr ( op not_expr )?
//   where op ∈ { ==, !=, <, <=, >, >=, in }
//
// Non-chainable: at most one comparison per cmp_expr. `a == b == c`
// errors; grouping via parens (`(a == b) == c`) is fine.
inline std::unique_ptr<Node> ParseState::parseCmp() {
    auto lhs = parseNot();
    if (!lhs) return nullptr;
    skipWhitespace();
    const Op *op = tryMatchOp(kCmpOps);
    if (!op) return lhs;
    pos += op->text.size();
    skipWhitespace();
    auto rhs = parseNot();
    if (!rhs) return nullptr;
    auto node  = std::make_unique<Node>();
    node->kind = op->kind;
    node->lhs  = std::move(lhs);
    node->rhs  = std::move(rhs);
    return node;
}

// ── not_expr ──────────────────────────────────
//
// not_expr := "!" not_expr | primary
//
// Right-associative: `!!x` parses as Not(Not(x)). Whitespace between
// the '!' and its operand is allowed.
inline std::unique_ptr<Node> ParseState::parseNot() {
    if (peek() != '!') {
        return parsePrimary();
    }
    ++pos;                                  // consume '!'
    skipWhitespace();
    auto child = parseNot();                // recurse for chained '!'
    if (!child) return nullptr;
    auto node  = std::make_unique<Node>();
    node->kind = Node::Kind::Not;
    node->lhs  = std::move(child);
    return node;
}

// ── primary ───────────────────────────────────
//
// Dispatches by first non-whitespace char to a per-kind parser.
inline std::unique_ptr<Node> ParseState::parsePrimary() {
    const char first = peek();

    if (first == '(')   return parseParen();
    if (isAlpha(first)) return parseWordOrKeyword();
    if (first == '@')   return parseAtIdentifier();
    if (first == '"')   return parseString();

    const bool starts_number =
        isDigit(first) ||
        (first == '-' && pos + 1 < source.size() && isDigit(source[pos + 1]));
    if (starts_number) return parseNumber();

    // Also covers EOF — peek() returns '\0' and matches nothing above.
    fail(pos, "expected identifier or literal");
    return nullptr;
}

// ── "(" expr ")" ──────────────────────────────
inline std::unique_ptr<Node> ParseState::parseParen() {
    const size_t open_pos = pos;
    ++pos;                                  // consume '('
    skipWhitespace();
    auto inner = parseExpr();
    if (!inner) return nullptr;
    skipWhitespace();
    if (atEnd() || peek() != ')') {
        if (atEnd()) fail(open_pos, "unclosed '('");
        else         fail(pos,      "expected ')'");
        return nullptr;
    }
    ++pos;                                  // consume ')'
    return inner;
}

// ── bare_id, "true", "false" ──────────────────
inline std::unique_ptr<Node> ParseState::parseWordOrKeyword() {
    const size_t word_start = pos;
    while (!atEnd() && isAlnum(peek())) ++pos;
    std::string_view word = source.substr(word_start, pos - word_start);

    auto node = std::make_unique<Node>();
    if (word == "true") {
        node->kind     = Node::Kind::LitBool;
        node->bool_val = true;
    } else if (word == "false") {
        node->kind     = Node::Kind::LitBool;
        node->bool_val = false;
    } else {
        node->kind    = Node::Kind::Identifier;
        node->str_val = std::string(word);
        addRef(node->str_val);
    }
    return node;
}

// ── "@" bare_id ───────────────────────────────
inline std::unique_ptr<Node> ParseState::parseAtIdentifier() {
    const size_t at_pos = pos;
    ++pos;                                  // consume '@'
    if (atEnd() || !isAlpha(peek())) {
        fail(at_pos, "expected identifier after '@'");
        return nullptr;
    }
    while (!atEnd() && isAlnum(peek())) ++pos;

    auto node = std::make_unique<Node>();
    node->kind    = Node::Kind::Identifier;
    node->str_val = std::string(source.substr(at_pos, pos - at_pos));   // includes '@'
    addRef(node->str_val);
    return node;
}

// ── -? [0-9]+ ( "." [0-9]+ )? ─────────────────
inline std::unique_ptr<Node> ParseState::parseNumber() {
    const size_t num_start = pos;
    if (peek() == '-') ++pos;
    while (!atEnd() && isDigit(peek())) ++pos;
    if (!atEnd() && peek() == '.') {
        const size_t dot_pos    = pos;
        ++pos;
        const size_t frac_start = pos;
        while (!atEnd() && isDigit(peek())) ++pos;
        if (pos == frac_start) {
            fail(dot_pos, "expected digits after '.'");
            return nullptr;
        }
    }

    // Copy the lexed slice into a NUL-terminated buffer for strtod.
    // The lexer has already validated the format, so strtod consuming
    // the entire buffer is defensive — anything else means lexer/parser
    // drift.
    std::string buf(source.substr(num_start, pos - num_start));
    char  *end = nullptr;
    double v   = std::strtod(buf.c_str(), &end);
    if (end != buf.c_str() + buf.size()) {
        fail(num_start, "invalid number");
        return nullptr;
    }

    auto node = std::make_unique<Node>();
    node->kind    = Node::Kind::LitNumber;
    node->num_val = v;
    return node;
}

// ── "([^"\\]|\\.)*" ───────────────────────────
inline std::unique_ptr<Node> ParseState::parseString() {
    const size_t str_start = pos;
    ++pos;                                  // consume opening quote
    std::string  decoded;
    bool         closed = false;

    while (!atEnd()) {
        const char ch = peek();
        if (ch == '"') {
            closed = true;
            ++pos;
            break;
        }
        if (ch == '\\') {
            ++pos;
            if (atEnd()) break;             // → unterminated below
            const char esc = peek();
            ++pos;
            switch (esc) {
                case '"':  decoded.push_back('"');  break;
                case '\\': decoded.push_back('\\'); break;
                case 'n':  decoded.push_back('\n'); break;
                case 't':  decoded.push_back('\t'); break;
                case 'r':  decoded.push_back('\r'); break;
                default:
                    fail(pos - 1, "invalid escape");    // offending escape char
                    return nullptr;
            }
            continue;
        }
        decoded.push_back(ch);
        ++pos;
    }

    if (!closed) {
        fail(str_start, "unterminated string");
        return nullptr;
    }

    auto node = std::make_unique<Node>();
    node->kind    = Node::Kind::LitString;
    node->str_val = std::move(decoded);
    return node;
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

    auto node = p.parseExpr();
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
