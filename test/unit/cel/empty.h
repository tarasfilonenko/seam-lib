#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/empty — empty / whitespace source + lifecycle invariants
//
// Covers Expression's always-true semantics for empty input, plus
// reset-on-re-use of `out` and `err` across compile() calls.
// ─────────────────────────────────────────────

#include "helpers.h"

test(cel_default_constructed_is_always_true) {
    seam::cel::Expression expr;
    assertTrue(expr.isAlwaysTrue());
    assertEqual((size_t)0, expr.references().size());
}

test(cel_compile_empty_source_succeeds) {
    auto c = cel_test::compileSrc("");
    assertTrue(c.ok);
    assertTrue(c.expr.isAlwaysTrue());
    assertEqual((size_t)0, c.expr.references().size());
    assertEqual((size_t)0, c.err.position);
    assertEqual("", c.err.message.c_str());
}

test(cel_compile_whitespace_source_succeeds) {
    auto c = cel_test::compileSrc("  \t  \r\n  ");
    assertTrue(c.ok);
    assertTrue(c.expr.isAlwaysTrue());
}

test(cel_evaluate_default_expression_returns_true) {
    seam::cel::Expression expr;
    seam::cel::Env        env = cel_test::emptyEnv();
    seam::cel::Value      v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isBool());
    assertTrue(v.asBool());
}

test(cel_evaluate_compiled_empty_source_returns_true) {
    cel_expect_bool("", true);
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
