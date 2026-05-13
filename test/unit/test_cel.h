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

// ── Step 3: number and string literals ────────────────────────

// Numbers — happy path
test(cel_compile_integer_zero) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("0", expr, err);
    assertTrue(ok);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(0.0, v.asNumber(), 1e-9);
}

test(cel_compile_positive_integer) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("42", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(42.0, v.asNumber(), 1e-9);
}

test(cel_compile_negative_integer) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("-7", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(-7.0, v.asNumber(), 1e-9);
}

test(cel_compile_decimal) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("3.14", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(3.14, v.asNumber(), 1e-9);
}

test(cel_compile_negative_decimal) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("-2.5", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(-2.5, v.asNumber(), 1e-9);
}

test(cel_compile_number_with_whitespace) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("  100  ", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(100.0, v.asNumber(), 1e-9);
}

test(cel_compile_negative_zero_evaluates_to_zero) {
    // -0 is a valid number literal; IEEE negative zero compares equal to
    // positive zero numerically, which is what we care about here.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("-0", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(0.0, v.asNumber(), 1e-9);
}

test(cel_compile_leading_zeros_are_decimal) {
    // Not octal — `007` is just the number 7. This matches strtod's
    // behaviour and avoids surprising module authors who might write
    // padded values.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("007", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isNumber());
    assertNear(7.0, v.asNumber(), 1e-9);
}

// Numbers — malformed
test(cel_compile_trailing_dot_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("1.", expr, err);
    assertFalse(ok);
    assertEqual((size_t)1, err.position);    // '.'
}

test(cel_compile_lone_minus_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("-", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

test(cel_compile_leading_dot_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile(".5", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

test(cel_compile_number_then_garbage_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("42 xyz", expr, err);
    assertFalse(ok);
    assertEqual((size_t)3, err.position);    // 'x'
}

test(cel_compile_double_minus_fails) {
    // `--5` — there is no unary minus operator in the grammar; the leading
    // sign is part of the number literal token, so `-` followed by `-` is
    // not a number start and there's no other primary that begins with `-`.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("--5", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

test(cel_compile_positive_sign_fails) {
    // `+5` — the grammar permits only an optional leading `-`. `+` is not
    // a primary start at all.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("+5", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

test(cel_compile_minus_then_plus_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("-+5", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

test(cel_compile_double_dot_fails) {
    // `1.5.5` — first `1.5` parses cleanly as a decimal, then the trailing
    // `.5` is unexpected content after the primary.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("1.5.5", expr, err);
    assertFalse(ok);
    assertEqual((size_t)3, err.position);    // second '.'
}

test(cel_compile_consecutive_dots_fails) {
    // `1..5` — after the first `.` the lexer demands at least one digit
    // before exiting the fractional part. Error points at the first dot
    // because that's where the malformed number begins.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("1..5", expr, err);
    assertFalse(ok);
    assertEqual((size_t)1, err.position);    // first '.'
}

test(cel_compile_scientific_notation_not_supported) {
    // `1e5` is not in the grammar. The number lexer consumes `1`, then
    // `e5` is trailing content. We don't try to recognise scientific
    // notation — caps values that need it should be expressed differently.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("1e5", expr, err);
    assertFalse(ok);
    assertEqual((size_t)1, err.position);    // 'e'
}

test(cel_compile_number_glued_to_identifier_fails) {
    // `123abc` — number lexer stops at `a`; the trailing word is not a
    // valid continuation of a primary.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("123abc", expr, err);
    assertFalse(ok);
    assertEqual((size_t)3, err.position);    // 'a'
}

// Strings — happy path
test(cel_compile_empty_string) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("\"\"", expr, err);
    assertTrue(ok);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual((size_t)0, v.asString().size());
}

test(cel_compile_simple_string) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("\"hello\"", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual("hello", std::string(v.asString()).c_str());
}

test(cel_compile_string_with_spaces) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("\"a b c\"", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual("a b c", std::string(v.asString()).c_str());
}

test(cel_compile_string_with_newline_escape) {
    // CEL source:  "line\nbreak"   ← \n is a CEL escape that decodes to LF
    // C++ literal: "\"line\\nbreak\""
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("\"line\\nbreak\"", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual("line\nbreak", std::string(v.asString()).c_str());
}

test(cel_compile_string_with_tab_and_cr_escapes) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("\"a\\tb\\rc\"", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual("a\tb\rc", std::string(v.asString()).c_str());
}

test(cel_compile_string_with_escaped_quote) {
    // CEL source:  "say \"hi\""
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("\"say \\\"hi\\\"\"", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual("say \"hi\"", std::string(v.asString()).c_str());
}

test(cel_compile_string_with_escaped_backslash) {
    // CEL source:  "back\\slash"
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    seam::cel::compile("\"back\\\\slash\"", expr, err);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual("back\\slash", std::string(v.asString()).c_str());
}

test(cel_compile_string_with_literal_newline_byte) {
    // The grammar's char class is `[^"\\]`, so a raw newline byte inside
    // a string is permitted and passes through unchanged. (Only an
    // unescaped quote or backslash terminate the body.)
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("\"a\nb\"", expr, err);
    assertTrue(ok);
    seam::cel::Env   env = cel_test::emptyEnv();
    seam::cel::Value v   = seam::cel::evaluate(expr, env);
    assertTrue(v.isString());
    assertEqual("a\nb", std::string(v.asString()).c_str());
}

// Strings — malformed
test(cel_compile_unterminated_string_fails) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("\"hello", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);    // opening quote
}

test(cel_compile_bad_escape_fails) {
    // "oops\xthing" — bad escape char 'x' at index 6
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("\"oops\\xthing\"", expr, err);
    assertFalse(ok);
    assertEqual((size_t)6, err.position);
}

test(cel_compile_trailing_backslash_fails) {
    // String ends with `\` and no following char → unterminated
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("\"oops\\", expr, err);
    assertFalse(ok);
    assertEqual((size_t)0, err.position);
}

test(cel_compile_string_with_zero_escape_fails) {
    // `\0` is not in the supported escape set (\", \\, \n, \t, \r). The
    // evaluator never sees embedded NUL anyway because Value uses
    // std::string, but rejecting it loudly at compile time avoids
    // ambiguity.
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    bool ok = seam::cel::compile("\"x\\0y\"", expr, err);
    assertFalse(ok);
    assertEqual((size_t)3, err.position);    // the '0' after '\'
}

// ── Steps 4+ go here ──────────────────────────────────────────
