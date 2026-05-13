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
// simple as features land, and don't allocate (Node owns nothing yet
// except children).
//
// Field reuse: `str_val` carries the decoded string body for LitString
// AND the identifier name (including any '@' prefix) for Identifier.
// `kind` discriminates.
// ─────────────────────────────────────────────

#include <cstdint>
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
    };

    Kind        kind     = Kind::LitBool;
    bool        bool_val = false;
    double      num_val  = 0.0;
    std::string str_val;        // LitString body OR Identifier name
};

} // namespace detail
} // namespace cel
} // namespace seam
