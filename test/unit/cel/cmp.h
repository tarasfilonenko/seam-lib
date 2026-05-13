#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/cmp — comparison operators
//
// cmp_expr := not_expr ( ("=="|"!="|"<"|"<="|">"|">=") not_expr )?
//
// At most ONE comparison per cmp_expr — `a == b == c` is a parse error
// without explicit parens. Grouping via `(a == b) == c` is allowed.
//
// Type-strict semantics:
//   - == / !=  require operands of the same kind (Bool/Number/String).
//              Mismatched types → Undefined (not false).
//   - < <= > >= require BOTH operands to be Number. Anything else
//              (string, bool, mixed) → Undefined.
//   - Any Undefined operand → Undefined.
// ─────────────────────────────────────────────

#include "helpers.h"

// Equality — numbers
test(cel_eval_eq_numbers_true)   { cel_expect_bool("1 == 1", true);  }
test(cel_eval_eq_numbers_false)  { cel_expect_bool("1 == 2", false); }
test(cel_eval_neq_numbers_true)  { cel_expect_bool("1 != 2", true);  }
test(cel_eval_neq_numbers_false) { cel_expect_bool("1 != 1", false); }
test(cel_eval_eq_decimals)       { cel_expect_bool("3.14 == 3.14", true); }
test(cel_eval_eq_negative)       { cel_expect_bool("-2 == -2", true); }
test(cel_eval_eq_zero_and_neg_zero) {
    // IEEE: +0 == -0
    cel_expect_bool("0 == -0", true);
}

// Equality — strings
test(cel_eval_eq_strings_true)  { cel_expect_bool("\"a\" == \"a\"", true);  }
test(cel_eval_eq_strings_false) { cel_expect_bool("\"a\" == \"b\"", false); }
test(cel_eval_eq_empty_strings) { cel_expect_bool("\"\" == \"\"",   true);  }

// Equality — bools
test(cel_eval_eq_bools_true)  { cel_expect_bool("true == true",   true);  }
test(cel_eval_eq_bools_false) { cel_expect_bool("true == false",  false); }
test(cel_eval_neq_bools)      { cel_expect_bool("true != false",  true);  }

// Ordering — numbers
test(cel_eval_lt_true)  { cel_expect_bool("1 < 2",  true);  }
test(cel_eval_lt_false) { cel_expect_bool("2 < 1",  false); }
test(cel_eval_lt_eq)    { cel_expect_bool("1 < 1",  false); }
test(cel_eval_le_eq)    { cel_expect_bool("2 <= 2", true);  }
test(cel_eval_le_less)  { cel_expect_bool("1 <= 2", true);  }
test(cel_eval_le_more)  { cel_expect_bool("3 <= 2", false); }
test(cel_eval_gt_true)  { cel_expect_bool("2 > 1",  true);  }
test(cel_eval_gt_false) { cel_expect_bool("1 > 2",  false); }
test(cel_eval_ge_eq)    { cel_expect_bool("2 >= 2", true);  }
test(cel_eval_ge_more)  { cel_expect_bool("3 >= 2", true);  }
test(cel_eval_ge_less)  { cel_expect_bool("1 >= 2", false); }

// Type-strict equality — mismatched kinds → Undefined (not false)
test(cel_eval_eq_number_vs_string_undefined) { cel_expect_undefined("1 == \"1\"");      }
test(cel_eval_eq_string_vs_number_undefined) { cel_expect_undefined("\"1\" == 1");      }
test(cel_eval_eq_bool_vs_number_undefined)   { cel_expect_undefined("true == 1");        }
test(cel_eval_eq_bool_vs_string_undefined)   { cel_expect_undefined("true == \"true\""); }
test(cel_eval_neq_mismatched_undefined)      { cel_expect_undefined("1 != \"1\"");      }

// Type-strict ordering — non-Number operands → Undefined
test(cel_eval_lt_strings_undefined) { cel_expect_undefined("\"a\" < \"b\""); }
test(cel_eval_lt_bools_undefined)   { cel_expect_undefined("true < false");  }
test(cel_eval_lt_mixed_undefined)   { cel_expect_undefined("1 < \"x\"");     }
test(cel_eval_gt_strings_undefined) { cel_expect_undefined("\"b\" > \"a\""); }

// Undefined propagation
test(cel_eval_eq_undefined_lhs_is_undefined) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing == 1", reg);
}

test(cel_eval_eq_undefined_rhs_is_undefined) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("1 == missing", reg);
}

test(cel_eval_lt_undefined_propagates) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing < 5", reg);
}

// Real-world shape: identifier compared to literal
test(cel_eval_param_eq_string_literal) {
    // The seam idiom: `mode == "advanced"`
    cel_test::Registry reg{{ { "mode", seam::cel::Value::string("advanced") } }};
    cel_expect_bool_in("mode == \"advanced\"", reg, true);
}

test(cel_eval_param_eq_string_literal_false) {
    cel_test::Registry reg{{ { "mode", seam::cel::Value::string("simple") } }};
    cel_expect_bool_in("mode == \"advanced\"", reg, false);
}

test(cel_eval_param_gt_zero) {
    // Another seam idiom: `gain > 0`
    cel_test::Registry reg{{ { "gain", seam::cel::Value::number(3.5) } }};
    cel_expect_bool_in("gain > 0", reg, true);
}

test(cel_eval_param_gt_zero_false) {
    cel_test::Registry reg{{ { "gain", seam::cel::Value::number(-1.0) } }};
    cel_expect_bool_in("gain > 0", reg, false);
}

// Whitespace + grouping
test(cel_eval_eq_extra_whitespace) { cel_expect_bool("1   ==   1", true); }
test(cel_eval_eq_with_parens_lhs)  { cel_expect_bool("(1) == 1",   true); }
test(cel_eval_eq_with_parens_rhs)  { cel_expect_bool("1 == (1)",   true); }
test(cel_eval_eq_with_parens_both) { cel_expect_bool("(1) == (1)", true); }

test(cel_eval_chained_eq_via_parens) {
    // (a == b) gives bool; that bool == c only when c is bool too.
    cel_expect_bool("(1 == 1) == true",  true);
    cel_expect_bool("(1 == 2) == false", true);
}

// '!' binds tighter than '==' (C / CEL precedence)
test(cel_eval_not_then_eq) {
    // !muted == false  →  (!muted) == false
    cel_test::Registry reg{{ { "muted", seam::cel::Value::boolean(true) } }};
    // muted=true → !muted=false → false == false → true
    cel_expect_bool_in("!muted == false", reg, true);
}

test(cel_eval_not_around_cmp) {
    // !(muted == true) → muted=true → muted==true=true → !true=false
    cel_test::Registry reg{{ { "muted", seam::cel::Value::boolean(true) } }};
    cel_expect_bool_in("!(muted == true)", reg, false);
}

// References — both operands tracked, with de-dup
test(cel_compile_eq_tracks_both_operands) {
    auto c = cel_test::compileSrc("a == b");
    assertTrue(c.ok);
    assertEqual((size_t)2, c.expr.references().size());
    assertEqual("a", c.expr.references()[0].c_str());
    assertEqual("b", c.expr.references()[1].c_str());
}

test(cel_compile_eq_dedups_same_identifier) {
    auto c = cel_test::compileSrc("a == a");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("a", c.expr.references()[0].c_str());
}

// Not chainable without parens
test(cel_compile_chained_eq_fails) {
    // `1 == 2 == 3` — second `==` is trailing content after the first cmp.
    cel_expect_compile_error("1 == 2 == 3", 7);     // '=' of second op
}

test(cel_compile_chained_lt_fails) {
    cel_expect_compile_error("1 < 2 < 3", 6);       // '<' of second op
}

// Malformed
test(cel_compile_single_equals_fails) {
    // `1 = 2` — '=' is not an operator; '1' parses, then trailing junk.
    cel_expect_compile_error("1 = 2", 2);           // '='
}

test(cel_compile_eq_missing_rhs_fails) {
    cel_expect_compile_error("1 ==", 4);            // EOF after '=='
}

test(cel_compile_lt_missing_rhs_fails) {
    cel_expect_compile_error("1 <", 3);             // EOF after '<'
}

test(cel_compile_eq_missing_lhs_fails) {
    cel_expect_compile_error("== 1", 0);            // '=' isn't a primary start
}

test(cel_compile_double_op_fails) {
    cel_expect_compile_error("1 < < 2", 4);         // second '<' isn't a primary
}
