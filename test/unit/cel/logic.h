#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/logic — logical && and ||
//
// or_expr  := and_expr ( "||" and_expr )*
// and_expr := cmp_expr ( "&&" cmp_expr )*
//
// Both are left-associative chains. Three-valued logic with error
// absorption (CEL convention):
//   && : false in EITHER operand → false (even if the other is
//        Undefined or non-bool — the false short-circuits the result)
//   || : true  in EITHER operand → true (same absorption)
//   anything else with a non-bool, non-Undefined operand → Undefined
//
// Precedence: '&&' binds tighter than '||' (C convention).
// ─────────────────────────────────────────────

#include "helpers.h"

// && truth table
test(cel_eval_and_tt) { cel_expect_bool("true && true",   true);  }
test(cel_eval_and_tf) { cel_expect_bool("true && false",  false); }
test(cel_eval_and_ft) { cel_expect_bool("false && true",  false); }
test(cel_eval_and_ff) { cel_expect_bool("false && false", false); }

// || truth table
test(cel_eval_or_tt)  { cel_expect_bool("true || true",   true);  }
test(cel_eval_or_tf)  { cel_expect_bool("true || false",  true);  }
test(cel_eval_or_ft)  { cel_expect_bool("false || true",  true);  }
test(cel_eval_or_ff)  { cel_expect_bool("false || false", false); }

// Error absorption — Undefined operand
test(cel_eval_and_false_absorbs_undef_rhs) {
    // false && missing → false (false short-circuits even though rhs is Undefined)
    cel_test::Registry reg{};
    cel_expect_bool_in("false && missing", reg, false);
}

test(cel_eval_and_false_absorbs_undef_lhs) {
    cel_test::Registry reg{};
    cel_expect_bool_in("missing && false", reg, false);
}

test(cel_eval_and_true_with_undef_is_undef) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("true && missing", reg);
}

test(cel_eval_and_undef_with_true_is_undef) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing && true", reg);
}

test(cel_eval_or_true_absorbs_undef_rhs) {
    cel_test::Registry reg{};
    cel_expect_bool_in("true || missing", reg, true);
}

test(cel_eval_or_true_absorbs_undef_lhs) {
    cel_test::Registry reg{};
    cel_expect_bool_in("missing || true", reg, true);
}

test(cel_eval_or_false_with_undef_is_undef) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("false || missing", reg);
}

test(cel_eval_or_undef_with_false_is_undef) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing || false", reg);
}

test(cel_eval_and_both_undef_is_undef) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing && other", reg);
}

test(cel_eval_or_both_undef_is_undef) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing || other", reg);
}

// Type strict + absorption — non-bool operands
test(cel_eval_and_number_absorbed_by_false) {
    // 1 && false → false (false absorbs even though lhs isn't bool)
    cel_expect_bool("1 && false", false);
}

test(cel_eval_and_false_absorbs_number_rhs) {
    cel_expect_bool("false && 1", false);
}

test(cel_eval_and_number_with_true_is_undef) {
    // No absorption: number lhs, true rhs — type error wins.
    cel_expect_undefined("1 && true");
}

test(cel_eval_and_true_with_number_is_undef) {
    cel_expect_undefined("true && 1");
}

test(cel_eval_and_number_with_number_is_undef) {
    cel_expect_undefined("1 && 2");
}

test(cel_eval_or_number_absorbed_by_true) {
    cel_expect_bool("1 || true", true);
}

test(cel_eval_or_true_absorbs_number_rhs) {
    cel_expect_bool("true || 1", true);
}

test(cel_eval_or_number_with_false_is_undef) {
    cel_expect_undefined("1 || false");
}

test(cel_eval_or_false_with_number_is_undef) {
    cel_expect_undefined("false || 1");
}

// Chains (left-associative)
test(cel_eval_and_chain_all_true)   { cel_expect_bool("true && true && true",   true);  }
test(cel_eval_and_chain_last_false) { cel_expect_bool("true && true && false",  false); }
test(cel_eval_and_chain_first_false) {
    // false in the chain absorbs anything to the right, even Undefined.
    cel_test::Registry reg{};
    cel_expect_bool_in("false && missing && other", reg, false);
}

test(cel_eval_or_chain_all_false)  { cel_expect_bool("false || false || false", false); }
test(cel_eval_or_chain_last_true)  { cel_expect_bool("false || false || true",  true);  }
test(cel_eval_or_chain_first_true) {
    cel_test::Registry reg{};
    cel_expect_bool_in("true || missing || other", reg, true);
}

// Precedence: && tighter than ||
test(cel_eval_and_tighter_than_or_a) {
    // true && false || true == (true && false) || true == false || true == true
    cel_expect_bool("true && false || true", true);
}

test(cel_eval_and_tighter_than_or_b) {
    // false || true && false == false || (true && false) == false || false == false
    cel_expect_bool("false || true && false", false);
}

// Combined with comparisons (comparisons tighter than &&)
test(cel_eval_and_with_comparisons) {
    cel_expect_bool("1 == 1 && 2 == 2", true);
}

test(cel_eval_or_with_comparisons) {
    cel_expect_bool("1 == 2 || 2 == 2", true);
}

test(cel_eval_and_or_with_comparisons) {
    // 1 == 1 && 2 == 3 || 3 == 3
    // = (1==1) && (2==3) || (3==3) = true && false || true = false || true = true
    cel_expect_bool("1 == 1 && 2 == 3 || 3 == 3", true);
}

// Real-world seam shapes
test(cel_eval_seam_idiom_two_conditions) {
    // mode == "advanced" && gain > 0
    cel_test::Registry reg{{
        { "mode", seam::cel::Value::string("advanced") },
        { "gain", seam::cel::Value::number(3.5) },
    }};
    cel_expect_bool_in("mode == \"advanced\" && gain > 0", reg, true);
}

test(cel_eval_seam_idiom_one_fails) {
    cel_test::Registry reg{{
        { "mode", seam::cel::Value::string("advanced") },
        { "gain", seam::cel::Value::number(-1.0) },
    }};
    cel_expect_bool_in("mode == \"advanced\" && gain > 0", reg, false);
}

// '!' precedence: tighter than && / ||
test(cel_eval_not_tighter_than_and) {
    // !true && false == (!true) && false = false && false = false
    cel_expect_bool("!true && false", false);
}

test(cel_eval_not_tighter_than_or) {
    // !false || false == (!false) || false = true || false = true
    cel_expect_bool("!false || false", true);
}

// Parens override precedence
test(cel_eval_paren_groups_or_first) {
    // (true || false) && false = true && false = false
    cel_expect_bool("(true || false) && false", false);
}

test(cel_eval_not_around_and) {
    // !(false && true) = !false = true
    cel_expect_bool("!(false && true)", true);
}

test(cel_eval_not_around_or) {
    // !(true || false) = !true = false
    cel_expect_bool("!(true || false)", false);
}

// References — tracked through && / ||, de-duped
test(cel_compile_and_tracks_both_operands) {
    auto c = cel_test::compileSrc("a && b");
    assertTrue(c.ok);
    assertEqual((size_t)2, c.expr.references().size());
}

test(cel_compile_and_dedups_same_identifier) {
    auto c = cel_test::compileSrc("a && a");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
}

test(cel_compile_chain_tracks_all_unique) {
    auto c = cel_test::compileSrc("a && b || c && a");
    assertTrue(c.ok);
    assertEqual((size_t)3, c.expr.references().size());     // a, b, c — a de-duped
}

// Malformed
test(cel_compile_and_missing_rhs_fails)  { cel_expect_compile_error("true &&",   7); }
test(cel_compile_and_missing_lhs_fails)  { cel_expect_compile_error("&& true",   0); }
test(cel_compile_or_missing_rhs_fails)   { cel_expect_compile_error("true ||",   7); }
test(cel_compile_or_missing_lhs_fails)   { cel_expect_compile_error("|| true",   0); }

test(cel_compile_single_ampersand_fails) {
    // '&' alone isn't an operator. `true & true` parses 'true', then '&' is
    // unexpected trailing content.
    cel_expect_compile_error("true & true", 5);   // '&'
}

test(cel_compile_single_pipe_fails) {
    cel_expect_compile_error("true | true", 5);   // '|'
}

test(cel_compile_triple_ampersand_fails) {
    // `a && && b` — the second '&&' tries to be parsed as a primary by
    // parseCmp's rhs, fails at the '&'.
    cel_expect_compile_error("a && && b", 5);     // second '&' of second '&&'
}

test(cel_compile_and_then_close_paren_fails) {
    cel_expect_compile_error("true && )", 8);     // ')' isn't a primary
}
