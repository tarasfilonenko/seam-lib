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
//   - Unary '!' requires a bool operand; non-bool / Undefined → Undefined.
//   - == / !=  require operands of the same kind (Bool/Number/String).
//              Mismatched types → Undefined (not false).
//   - < <= > >= require BOTH operands to be Number. Anything else
//              (string/bool/mixed) → Undefined.
//   - && / ||  use three-valued logic with error absorption:
//                 false  absorbs in &&  → false even if other is Undef/non-bool
//                 true   absorbs in ||  → true  even if other is Undef/non-bool
//              Non-absorbing combinations of non-bool / Undefined → Undefined.
//   - 'in' requires a List on the right (CEL-strict). Element-wise
//              equality drives membership; one matching element → true,
//              otherwise Undefined if any per-element comparison was
//              Undefined, otherwise false. Non-List right operand →
//              Undefined.
//   - List literals `[a, b, c]` evaluate each element eagerly. List
//              values flow naturally through 'in' but aren't otherwise
//              comparable today (`[1] == [1]` → Undefined).
//   - The evaluator is total: no exceptions, no UB on bad input. Bad
//     runtime types collapse to Undefined and callers wrap evaluate()
//     in tight loops without try/catch overhead.
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

#include <memory>
#include <vector>

#include "Env.h"
#include "Expression.h"
#include "Value.h"
#include "detail/Node.h"

namespace seam {
namespace cel {

namespace detail {

// Forward-decl so evalCmp and evaluateNode can call each other.
inline Value evaluateNode(const Node *node, const Env &env);

// Evaluate a binary comparison given already-resolved operand Values.
// Centralises the type-strict rules so the recursive walker stays
// readable.
inline Value evalCmp(Node::Kind op, const Value &l, const Value &r) {
    if (l.isUndefined() || r.isUndefined()) {
        return Value::undefined();
    }

    // Equality / inequality: same kind required; bool/number/string each
    // compare with their own ==. Mismatched kinds (or List operands,
    // which we don't support equality for today) → Undefined.
    if (op == Node::Kind::Eq || op == Node::Kind::Neq) {
        if (l.kind() != r.kind()) {
            return Value::undefined();
        }
        bool equal = false;
        switch (l.kind()) {
            case Value::Kind::Bool:   equal = l.asBool()   == r.asBool();   break;
            case Value::Kind::Number: equal = l.asNumber() == r.asNumber(); break;
            case Value::Kind::String: equal = l.asString() == r.asString(); break;
            default:                  return Value::undefined();   // List, etc.
        }
        return Value::boolean(op == Node::Kind::Eq ? equal : !equal);
    }

    // 'in': right operand must be a List. Element-wise equality drives
    // membership. CEL-style error absorption: a single matching element
    // wins (returns true) regardless of any earlier Undefined comparisons;
    // otherwise an Undefined comparison along the way propagates if no
    // match is found.
    if (op == Node::Kind::In) {
        if (!r.isList()) {
            return Value::undefined();
        }
        auto list = r.asList();
        if (!list) {
            return Value::undefined();
        }
        bool saw_undef = false;
        for (const auto &elem : list->items) {
            const Value eq = evalCmp(Node::Kind::Eq, l, elem);
            if (eq.isBool() && eq.asBool()) {
                return Value::boolean(true);
            }
            if (eq.isUndefined()) {
                saw_undef = true;
            }
        }
        return saw_undef ? Value::undefined() : Value::boolean(false);
    }

    // Ordering: both must be Number. Anything else → Undefined.
    if (!l.isNumber() || !r.isNumber()) {
        return Value::undefined();
    }
    const double a = l.asNumber();
    const double b = r.asNumber();
    switch (op) {
        case Node::Kind::Lt: return Value::boolean(a <  b);
        case Node::Kind::Le: return Value::boolean(a <= b);
        case Node::Kind::Gt: return Value::boolean(a >  b);
        case Node::Kind::Ge: return Value::boolean(a >= b);
        default:             return Value::undefined();   // unreachable
    }
}

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
        case Node::Kind::ListLit: {
            // Evaluate each element eagerly; element Undefined-ness is
            // preserved (so `unbound in [1, 2]` still returns Undefined
            // via the In branch — the comparison sees an Undef left).
            std::vector<Value> items;
            items.reserve(node->children.size());
            for (const auto &child : node->children) {
                items.push_back(evaluateNode(child.get(), env));
            }
            return Value::list(std::move(items));
        }
        case Node::Kind::Not: {
            Value v = evaluateNode(node->lhs.get(), env);
            if (v.isUndefined()) return Value::undefined();
            if (!v.isBool())     return Value::undefined();   // type-strict
            return Value::boolean(!v.asBool());
        }
        case Node::Kind::Eq:
        case Node::Kind::Neq:
        case Node::Kind::Lt:
        case Node::Kind::Le:
        case Node::Kind::Gt:
        case Node::Kind::Ge:
        case Node::Kind::In: {
            Value l = evaluateNode(node->lhs.get(), env);
            Value r = evaluateNode(node->rhs.get(), env);
            return evalCmp(node->kind, l, r);
        }
        case Node::Kind::And: {
            // CEL three-valued with error absorption: a definitive `false`
            // in either operand makes the result false, regardless of the
            // other operand's type or undefined-ness. Otherwise both must
            // be bool to get a true; anything else → Undefined.
            Value l = evaluateNode(node->lhs.get(), env);
            Value r = evaluateNode(node->rhs.get(), env);
            if (l.isBool() && !l.asBool()) return Value::boolean(false);
            if (r.isBool() && !r.asBool()) return Value::boolean(false);
            if (l.isBool() && r.isBool()) return Value::boolean(l.asBool() && r.asBool());
            return Value::undefined();
        }
        case Node::Kind::Or: {
            // Mirror of &&: definitive `true` absorbs.
            Value l = evaluateNode(node->lhs.get(), env);
            Value r = evaluateNode(node->rhs.get(), env);
            if (l.isBool() && l.asBool()) return Value::boolean(true);
            if (r.isBool() && r.asBool()) return Value::boolean(true);
            if (l.isBool() && r.isBool()) return Value::boolean(l.asBool() || r.asBool());
            return Value::undefined();
        }
    }
    return Value::undefined();      // unreachable today; defensive for future kinds
}

} // namespace detail

inline Value evaluate(const Expression &expression, const Env &env) {
    return detail::evaluateNode(expression._root.get(), env);
}

} // namespace cel
} // namespace seam
