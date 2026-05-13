#pragma once
// ─────────────────────────────────────────────
// seam::cel::Expression
//
// Compiled form of a SEAM-CEL source string.
//
// Lifecycle:
//   1. seam::cel::compile(source, out, err)  — parses source into Expression.
//   2. expression.references()                — set of identifiers it reads,
//                                                used to build reverse-dep
//                                                indices ("which expressions
//                                                care about param X?").
//   3. seam::cel::evaluate(expression, env)   — runs against an Env, returns
//                                                a Value (typically Bool, or
//                                                Undefined if any referenced
//                                                identifier is unbound).
//
// An empty Expression (default-constructed, or compiled from "" / pure
// whitespace) evaluates to Value::boolean(true). caps fields use empty ==
// "always" semantics, so callers can compile() every visible_expr /
// enabled_expr uniformly without special-casing the empty case.
//
// Move-only. Cheap to move, expensive to copy — copy is intentionally
// disabled to keep ownership of the underlying AST unambiguous.
//
// Note for test-sketch authors: this header pulls in <memory>, which
// transitively includes <atomic>. AUnit's `test()` macro collides with
// std::atomic_flag::test(), so any sketch that uses both must include
// <atomic> (or any cel/seam-lib header that pulls it in) BEFORE
// <AUnit.h>. See lib/seam-lib/test/unit/unit.ino for the canonical fix.
// ─────────────────────────────────────────────

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace seam {
namespace cel {

class Value;
struct Env;
struct CompileError;

namespace detail { struct Node; }

class Expression {
public:
    Expression() noexcept;
    ~Expression();

    Expression(Expression &&) noexcept;
    Expression &operator=(Expression &&) noexcept;

    Expression(const Expression &) = delete;
    Expression &operator=(const Expression &) = delete;

    // True when no AST is attached: the expression came from empty / pure
    // whitespace source, or was default-constructed. Always-true expressions
    // evaluate to Value::boolean(true).
    bool isAlwaysTrue() const noexcept { return !_root; }

    // Unordered, de-duplicated list of identifiers this expression reads.
    // Empty for always-true expressions. Stable across the Expression's
    // lifetime — safe to take pointers/refs into it for reverse-dep indices.
    const std::vector<std::string> &references() const noexcept { return _references; }

private:
    std::unique_ptr<detail::Node> _root;          // null ⇒ always-true
    std::vector<std::string>      _references;

    // compile() builds _root / _references; evaluate() walks _root.
    friend bool  compile(std::string_view, Expression &, CompileError &);
    friend Value evaluate(const Expression &, const Env &);
};

} // namespace cel
} // namespace seam

// detail::Node must be a complete type before std::unique_ptr<detail::Node>
// can be destructed or moved. Pull it in here so the inline special-member
// defaults below see a complete type.
#include "detail/Node.h"

namespace seam {
namespace cel {

inline Expression::Expression() noexcept                       = default;
inline Expression::~Expression()                               = default;
inline Expression::Expression(Expression &&) noexcept          = default;
inline Expression &Expression::operator=(Expression &&) noexcept = default;

} // namespace cel
} // namespace seam
