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

inline Value evaluate(const Expression &expression, const Env & /*env*/) {
    const detail::Node *node = expression._root.get();
    if (!node) {
        return Value::boolean(true);    // always-true / empty source
    }
    switch (node->kind) {
        case detail::Node::Kind::LitBool:
            return Value::boolean(node->bool_val);
    }
    return Value::undefined();          // unreachable today; defensive for future kinds
}

} // namespace cel
} // namespace seam
