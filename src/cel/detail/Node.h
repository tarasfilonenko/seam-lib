#pragma once
// ─────────────────────────────────────────────
// seam::cel::detail::Node
//
// AST node for compiled SEAM-CEL expressions. Internal — the only stable
// surface for users is Expression, compile(), evaluate(). Node grows as
// the grammar is implemented step-by-step.
//
// Defined as a complete type in this header so std::unique_ptr<Node>
// in Expression can be destructed/moved when this header is included
// (which Expression.h does, at the bottom, right before its inline
// special-member definitions).
//
// Layout choice: a single struct with all per-kind fields rather than a
// std::variant or class hierarchy. Each kind only uses a subset; the
// unused fields cost a few bytes per node but keep the grammar code
// simple as features land. Unary ops use `lhs`; binary ops use both
// `lhs` and `rhs`.
//
// Field reuse:
//   str_val  ─ LitString body OR Identifier name (incl. '@' prefix)
//   lhs      ─ unary operand (Not) OR binary left operand
//   rhs      ─ binary right operand (unused for unary)
// ─────────────────────────────────────────────

#include <cstdint>
#include <memory>
#include <string>

namespace seam {
namespace cel {
namespace detail {

struct Node {
    enum class Kind : uint8_t {
        LitBool,
        LitNumber,
        LitString,
        Identifier,
        Not,
        // Comparison (binary). Type-strict: see evaluator for rules.
        Eq,     // ==
        Neq,    // !=
        Lt,     // <
        Le,     // <=
        Gt,     // >
        Ge,     // >=
        // Logical (binary). Three-valued with error absorption: false
        // absorbs in &&, true absorbs in ||.
        And,    // &&
        Or,     // ||
        // Membership (binary). String-only: lhs in rhs where rhs is
        // treated as a space-separated token list.
        In,     // in
    };

    Kind        kind     = Kind::LitBool;
    bool        bool_val = false;
    double      num_val  = 0.0;
    std::string str_val;            // LitString body OR Identifier name

    std::unique_ptr<Node> lhs;      // unary operand (Not) OR binary left
    std::unique_ptr<Node> rhs;      // binary right (unused for unary)
};

} // namespace detail
} // namespace cel
} // namespace seam
