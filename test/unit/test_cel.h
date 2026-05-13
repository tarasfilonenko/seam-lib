#pragma once
// ─────────────────────────────────────────────
// test_cel — AUnit tests for seam::cel
//
// Grows step-by-step alongside the cel implementation. Each step adds
// failing tests first, then the impl that turns them green.
//
// Current coverage: step 1 — empty / pure-whitespace source becomes an
// always-true Expression that evaluates to Value::boolean(true).
// ─────────────────────────────────────────────

#include <AUnit.h>
#include "cel/cel.h"

namespace cel_test {

inline seam::cel::Value lookupAlwaysUndefined(void* /*ctx*/, std::string_view /*id*/) {
    return seam::cel::Value::undefined();
}

inline seam::cel::Env emptyEnv() {
    return seam::cel::Env{ nullptr, &lookupAlwaysUndefined };
}

} // namespace cel_test

// ── Step 1: empty / whitespace source → always-true ───────────

test(cel_default_constructed_is_always_true) {
    seam::cel::Expression expr;
    assertTrue(expr.isAlwaysTrue());
    assertEqual((size_t)0, expr.references().size());
}

test(cel_compile_empty_source_succeeds) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("", expr, err);
    assertTrue(ok);
    assertTrue(expr.isAlwaysTrue());
    assertEqual((size_t)0, expr.references().size());
    assertEqual((size_t)0, err.position);
    assertEqual("", err.message.c_str());
}

test(cel_compile_whitespace_source_succeeds) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("  \t  \r\n  ", expr, err);
    assertTrue(ok);
    assertTrue(expr.isAlwaysTrue());
}

test(cel_evaluate_default_expression_returns_true) {
    seam::cel::Expression expr;
    seam::cel::Env        env = cel_test::emptyEnv();
    seam::cel::Value      v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isBool());
    assertTrue(v.asBool());
}

test(cel_evaluate_compiled_empty_source_returns_true) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isBool());
    assertTrue(v.asBool());
}

test(cel_compile_resets_out_on_re_use) {
    // After a successful compile, re-compiling with empty source should
    // still leave the Expression in always-true state — i.e. compile()
    // must reset `out`, not append to it.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("", expr, err);
    seam::cel::compile("", expr, err);
    assertTrue(expr.isAlwaysTrue());
}

test(cel_compile_resets_err_on_success) {
    // err should be cleared on a successful compile so callers can
    // re-use the same CompileError variable across calls.
    seam::cel::Expression  expr;
    seam::cel::CompileError err{ 42, "stale" };
    bool ok = seam::cel::compile("", expr, err);
    assertTrue(ok);
    assertEqual((size_t)0, err.position);
    assertEqual("", err.message.c_str());
}

// ── Steps 2+ go here ──────────────────────────────────────────
