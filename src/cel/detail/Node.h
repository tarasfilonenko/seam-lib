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
// `lhs` and `rhs`; list literals use `children`.
//
// Field reuse:
//   str_val   ─ LitString body OR Identifier name (incl. '@' prefix)
//   lhs       ─ unary operand (Not) OR binary left operand
//   rhs       ─ binary right operand (unused for unary)
//   children  ─ list literal elements (ListLit only)
// ─────────────────────────────────────────────

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace seam {
namespace cel {
namespace detail {

struct Node {
    enum class Kind : uint8_t {
        LitBool,
        LitNumber,
        LitString,
        Identifier,
        ListLit,        // [expr, expr, ...]
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
        // Membership (binary). Right operand must be a List; element-wise
        // equality determines membership.
        In,     // in
    };

    Kind        kind     = Kind::LitBool;
    bool        bool_val = false;
    double      num_val  = 0.0;
    std::string str_val;            // LitString body OR Identifier name

    std::unique_ptr<Node>                 lhs;          // unary operand OR binary left
    std::unique_ptr<Node>                 rhs;          // binary right (unused for unary)
    std::vector<std::unique_ptr<Node>>    children;     // ListLit elements
};

} // namespace detail
} // namespace cel
} // namespace seam
