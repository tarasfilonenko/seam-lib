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
// An empty Expression (default-constructed, or compiled from "") evaluates
// to Value::boolean(true). caps fields use empty == "always" semantics, so
// callers can compile() every visible_expr/enabled_expr uniformly without
// special-casing the empty case.
//
// Move-only. Cheap to move, expensive to copy — copy is intentionally
// disabled to keep ownership of the underlying AST unambiguous.
// ─────────────────────────────────────────────

#include <memory>
#include <string>
#include <vector>

namespace seam {
namespace cel {

class Expression {
public:
    Expression();                                   // empty → always true
    ~Expression();

    Expression(Expression &&) noexcept;
    Expression &operator=(Expression &&) noexcept;

    Expression(const Expression &) = delete;
    Expression &operator=(const Expression &) = delete;

    // True when this expression was compiled from an empty source string
    // (or default-constructed). Such expressions evaluate to true.
    bool isAlwaysTrue() const;

    // Unordered, de-duplicated list of identifiers this expression reads.
    // Stable across the Expression's lifetime — safe to take pointers/refs
    // into it for reverse-dependency indices.
    const std::vector<std::string> &references() const;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;

    // Friend access so compile()/evaluate() can populate / inspect Impl
    // without leaking AST internals into the public header.
    friend class CompileAccess;
    friend class EvaluateAccess;
};

} // namespace cel
} // namespace seam
