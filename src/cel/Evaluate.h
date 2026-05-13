#pragma once
// ─────────────────────────────────────────────
// seam::cel::evaluate
//
// Runs a compiled Expression against an Env and returns a Value.
//
// Semantics:
//   - An empty / always-true Expression returns Value::boolean(true).
//   - Identifiers are resolved through env.resolve(). Resolvers that
//     return Value::undefined() cause the whole expression to evaluate
//     to Value::undefined() (Undefined propagates through every operator).
//   - Type mismatches at evaluation time (e.g. comparing a string to a
//     number) yield Value::undefined() rather than throwing — the
//     evaluator is total, so callers can wrap it in tight loops without
//     try/catch overhead.
//   - Unary '!' requires a bool operand; any other type → Undefined.
//
// Callers that want a boolean answer should typically do:
//
//     Value v = seam::cel::evaluate(expr, env);
//     if (v.isUndefined()) {
//         // not-ready: hide the row, don't disable it; re-evaluate later
//     } else {
//         bool ok = v.isBool() ? v.asBool() : true;   // fail-open
//     }
//
// This evaluator is pure: it reads from env, never writes. Safe to call
// from any task as long as the Env's lookup() is itself thread-safe.
// ─────────────────────────────────────────────

#include "Env.h"
#include "Expression.h"
#include "Value.h"
#include "detail/Node.h"

namespace seam {
namespace cel {

namespace detail {

// Recursive tree walker. Returns Value::boolean(true) for the empty
// (null) Node — matches always-true semantics. Undefined propagates
// through every operator branch.
inline Value evaluateNode(const Node *node, const Env &env) {
    if (!node) {
        return Value::boolean(true);
    }
    switch (node->kind) {
        case Node::Kind::LitBool:    return Value::boolean(node->bool_val);
        case Node::Kind::LitNumber:  return Value::number(node->num_val);
        case Node::Kind::LitString:  return Value::string(node->str_val);
        case Node::Kind::Identifier: return env.resolve(node->str_val);
        case Node::Kind::Not: {
            Value v = evaluateNode(node->lhs.get(), env);
            if (v.isUndefined()) return Value::undefined();
            if (!v.isBool())     return Value::undefined();   // type-strict
            return Value::boolean(!v.asBool());
        }
    }
    return Value::undefined();          // unreachable today; defensive for future kinds
}

} // namespace detail

inline Value evaluate(const Expression &expression, const Env &env) {
    return detail::evaluateNode(expression._root.get(), env);
}

} // namespace cel
} // namespace seam
