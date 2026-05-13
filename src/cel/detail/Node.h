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
// unused fields cost a few bytes per node but keep the code grammar
// simple as features land, and don't allocate (Node owns nothing yet
// except children).
// ─────────────────────────────────────────────

#include <cstdint>

namespace seam {
namespace cel {
namespace detail {

struct Node {
    enum class Kind : uint8_t {
        LitBool,
    };

    Kind kind     = Kind::LitBool;
    bool bool_val = false;
};

} // namespace detail
} // namespace cel
} // namespace seam
