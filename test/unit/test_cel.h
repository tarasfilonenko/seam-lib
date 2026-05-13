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

// ── Step 2: boolean literals ──────────────────────────────────

test(cel_compile_true_succeeds) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("true", expr, err);
    assertTrue(ok);
    assertFalse(expr.isAlwaysTrue());
    assertEqual((size_t)0, expr.references().size());
}

test(cel_compile_false_succeeds) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("false", expr, err);
    assertTrue(ok);
    assertFalse(expr.isAlwaysTrue());
}

test(cel_evaluate_true_returns_true) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("true", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isBool());
    assertTrue(v.asBool());
}

test(cel_evaluate_false_returns_false) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("false", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isBool());
    assertFalse(v.asBool());
}

test(cel_compile_bool_literal_with_surrounding_whitespace) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("  true  \n", expr, err);
    assertTrue(ok);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isBool());
    assertTrue(v.asBool());
}

test(cel_compile_trailing_garbage_fails_and_resets_expression) {
    // Anything after the literal (other than whitespace) is a parse error,
    // and on failure the Expression must be reset to always-true so the
    // caller's fail-open policy gives a sensible result.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("true xyz", expr, err);
    assertFalse(ok);
    assertTrue(expr.isAlwaysTrue());
    assertEqual((size_t)5, err.position);   // 'x'
}

test(cel_compile_glued_word_is_unknown_identifier) {
    // Lexer reads full identifier-like words before classifying, so
    // "truefalse" is one word that matches neither bool literal — should
    // error at the start of the word, not in the middle.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("truefalse", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

test(cel_compile_two_literals_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("true false", expr, err);
    assertFalse(ok);
    assertEqual((size_t)5, err.position);   // 'f' in "false"
}

test(cel_compile_leading_non_letter_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("@", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

// ── Steps 3+ go here ──────────────────────────────────────────
