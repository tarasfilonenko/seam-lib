#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/paren — parenthesised primary expressions
//
// `primary := ... | "(" expr ")"`. Parens are transparent — they wrap
// the inner expression for grouping and don't add an AST node. With no
// operators yet (steps 6+), they're equivalent to their content, but
// the recursive parser structure is what's being exercised here.
// ─────────────────────────────────────────────

#include "helpers.h"

// Happy path
test(cel_compile_parens_around_bool)   { cel_expect_bool("(true)",      true);  }
test(cel_compile_parens_around_number) { cel_expect_number("(42)",      42.0); }
test(cel_compile_parens_around_string) { cel_expect_string("(\"hi\")",  "hi");  }

test(cel_compile_parens_around_identifier) {
    cel_test::Registry reg{{ { "x", seam::cel::Value::number(7.0) } }};
    cel_expect_number_in("(x)", reg, 7.0);
}

test(cel_compile_parens_around_negative_number) {
    cel_expect_number("(-5)", -5.0);
}

test(cel_compile_nested_parens) {
    cel_expect_bool("((true))", true);
}

test(cel_compile_deeply_nested_parens) {
    cel_test::Registry reg{{ { "x", seam::cel::Value::number(42.0) } }};
    cel_expect_number_in("(((x)))", reg, 42.0);
}

test(cel_compile_parens_with_inner_whitespace) {
    cel_expect_bool("(  true  )", true);
}

test(cel_compile_parens_with_outer_whitespace) {
    cel_expect_bool("  (true)  ", true);
}

test(cel_compile_nested_parens_with_whitespace) {
    cel_expect_bool("(  (  true  )  )", true);
}

test(cel_compile_parens_dont_affect_references) {
    // Identifiers inside parens still register in references() and stay
    // de-duped — wrapping is purely syntactic.
    auto c = cel_test::compileSrc("((foo))");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("foo", c.expr.references()[0].c_str());
}

// Malformed
test(cel_compile_empty_parens_fail) {
    // `(` then `)` — `)` isn't a valid primary start; error at the ')'.
    cel_expect_compile_error("()", 1);
}

test(cel_compile_unclosed_paren_fails) {
    // Unclosed at EOF: error reports the position of the open '('
    // so the user can find the offending opener.
    cel_expect_compile_error("(true", 0);
}

test(cel_compile_unclosed_nested_paren_fails) {
    // Outer paren lacks its close. Error points at the outer '('.
    cel_expect_compile_error("((true)", 0);
}

test(cel_compile_paren_with_inner_garbage_fails) {
    // Inner expression parses ("true"), but trailing content inside
    // the parens isn't ')' → "expected ')'" at the offending byte.
    cel_expect_compile_error("(true xyz)", 6);   // 'x'
}

test(cel_compile_unmatched_close_fails) {
    // Trailing ')' after a complete primary outside any open paren.
    cel_expect_compile_error("true)", 4);
}

test(cel_compile_extra_close_after_balanced_parens_fails) {
    cel_expect_compile_error("(true))", 6);
}

test(cel_compile_just_open_paren_fails) {
    // Inner parsePrimary fails at EOF with "expected identifier or
    // literal" at position 1 (right after the '(').
    cel_expect_compile_error("(", 1);
}

test(cel_compile_just_close_paren_fails) {
    cel_expect_compile_error(")", 0);
}
