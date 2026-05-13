#pragma once
// ─────────────────────────────────────────────
// seam::cel::detail::Node
//
// AST node for compiled SEAM-CEL expressions. Internal — the only stable
// surface for users is Expression, compile(), evaluate(). Node grows as
// the grammar is implemented step-by-step:
//
//   step 1  ─ empty struct (only empty expressions parse; no Nodes are
//             ever constructed).
//   step 2+ ─ adds fields per grammar feature (literals, identifiers,
//             unary/binary ops, `in`).
//
// Defined as a complete type in this header so std::unique_ptr<Node>
// in Expression can be destructed/moved when this header is included
// (which Expression.h does, at the bottom, right before its inline
// special-member definitions).
// ─────────────────────────────────────────────

namespace seam {
namespace cel {
namespace detail {

struct Node {
    // Body grows as the grammar is implemented.
};

} // namespace detail
} // namespace cel
} // namespace seam
