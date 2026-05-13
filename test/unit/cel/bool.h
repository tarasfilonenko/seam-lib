#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/bool — boolean literals (true / false)
// ─────────────────────────────────────────────

#include "helpers.h"

test(cel_compile_true_succeeds) {
    auto c = cel_test::compileSrc("true");
    assertTrue(c.ok);
    assertFalse(c.expr.isAlwaysTrue());
    assertEqual((size_t)0, c.expr.references().size());
}

test(cel_compile_false_succeeds) {
    auto c = cel_test::compileSrc("false");
    assertTrue(c.ok);
    assertFalse(c.expr.isAlwaysTrue());
}

test(cel_evaluate_true_returns_true)   { cel_expect_bool("true",  true);  }
test(cel_evaluate_false_returns_false) { cel_expect_bool("false", false); }

test(cel_compile_bool_literal_with_surrounding_whitespace) {
    cel_expect_bool("  true  \n", true);
}

test(cel_compile_trailing_garbage_fails_and_resets_expression) {
    // Anything after the literal (other than whitespace) is a parse error,
    // and on failure the Expression must be reset to always-true so the
    // caller's fail-open policy gives a sensible result.
    auto c = cel_test::compileSrc("true xyz");
    assertFalse(c.ok);
    assertTrue(c.expr.isAlwaysTrue());
    assertEqual((size_t)5, c.err.position);     // 'x'
}

test(cel_compile_glued_word_is_identifier) {
    // Lexer reads full identifier-like words before classifying. An
    // unrecognised word becomes a plain identifier rather than a parse
    // error — so "truefalse" is the identifier `truefalse`, not "true"
    // glued to "false".
    auto c = cel_test::compileSrc("truefalse");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("truefalse", c.expr.references()[0].c_str());
}

test(cel_compile_two_literals_fails) {
    cel_expect_compile_error("true false", 5);     // 'f' in "false"
}

test(cel_compile_leading_non_letter_fails) {
    // '@' alone is a malformed @-identifier (no bare_id after).
    cel_expect_compile_error("@", 0);
}
