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
// AST ownership is via raw owning pointer rather than std::unique_ptr.
// This is deliberate: on libstdc++ for ESP32, <memory> transitively pulls
// in <atomic>, whose std::atomic_flag::test() method collides with the
// AUnit `test()` macro used in this library's own test sketch. The rest
// of seam-lib avoids <memory> for the same reason. Raw ownership keeps
// the public header AUnit-safe.
// ─────────────────────────────────────────────

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace seam {
namespace cel {

class Value;
struct Env;
struct CompileError;

namespace detail { struct Node; }

class Expression {
public:
    Expression() noexcept = default;
    ~Expression();

    Expression(Expression &&) noexcept;
    Expression &operator=(Expression &&) noexcept;

    Expression(const Expression &) = delete;
    Expression &operator=(const Expression &) = delete;

    // True when no AST is attached: the expression came from empty / pure
    // whitespace source, or was default-constructed. Always-true expressions
    // evaluate to Value::boolean(true).
    bool isAlwaysTrue() const noexcept { return _root == nullptr; }

    // Unordered, de-duplicated list of identifiers this expression reads.
    // Empty for always-true expressions. Stable across the Expression's
    // lifetime — safe to take pointers/refs into it for reverse-dep indices.
    const std::vector<std::string> &references() const noexcept { return _references; }

private:
    detail::Node             *_root = nullptr;   // owning, null ⇒ always-true
    std::vector<std::string>  _references;

    // compile() builds _root / _references; evaluate() walks _root.
    friend bool  compile(std::string_view, Expression &, CompileError &);
    friend Value evaluate(const Expression &, const Env &);
};

} // namespace cel
} // namespace seam

// detail::Node must be a complete type before the inline special-member
// definitions below can `delete` it. Pull it in here so the dtor + move
// definitions see a complete type.
#include "detail/Node.h"

namespace seam {
namespace cel {

inline Expression::~Expression() {
    delete _root;
}

inline Expression::Expression(Expression &&other) noexcept
    : _root(other._root)
    , _references(std::move(other._references))
{
    other._root = nullptr;
}

inline Expression &Expression::operator=(Expression &&other) noexcept {
    if (this != &other) {
        delete _root;
        _root        = other._root;
        other._root  = nullptr;
        _references  = std::move(other._references);
    }
    return *this;
}

} // namespace cel
} // namespace seam
