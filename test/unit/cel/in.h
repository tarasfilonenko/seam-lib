#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/in — 'in' membership operator
//
// `item in list` returns true if item equals some element of list,
// using the same type-strict equality rules as ==. Right operand must
// be a List (non-List → Undefined). CEL-style error absorption:
//   - any matching element → true (even if earlier comparisons were
//     Undefined)
//   - no match found and at least one Undefined comparison → Undefined
//   - no match found and all comparisons were defined-false → false
//
// 'in' is a keyword operator at cmp_expr precedence — same slot as ==.
// Non-chainable; use parens to group (`(a in [1]) == true`).
// ─────────────────────────────────────────────

#include "helpers.h"

// Happy path — basic membership
test(cel_eval_in_int_match_first)  { cel_expect_bool("1 in [1, 2, 3]", true);  }
test(cel_eval_in_int_match_middle) { cel_expect_bool("2 in [1, 2, 3]", true);  }
test(cel_eval_in_int_match_last)   { cel_expect_bool("3 in [1, 2, 3]", true);  }
test(cel_eval_in_int_no_match)     { cel_expect_bool("4 in [1, 2, 3]", false); }

test(cel_eval_in_string_match)     { cel_expect_bool("\"a\" in [\"a\", \"b\"]", true);  }
test(cel_eval_in_string_no_match)  { cel_expect_bool("\"c\" in [\"a\", \"b\"]", false); }

test(cel_eval_in_bool_match)       { cel_expect_bool("true in [true, false]", true);   }
test(cel_eval_in_bool_no_match)    { cel_expect_bool("false in [true]",       false);  }

// Edge cases
test(cel_eval_in_singleton_match)  { cel_expect_bool("1 in [1]",     true);  }
test(cel_eval_in_singleton_miss)   { cel_expect_bool("2 in [1]",     false); }
test(cel_eval_in_empty_list)       { cel_expect_bool("1 in []",      false); }

test(cel_eval_in_decimal_exact_match) {
    cel_expect_bool("3.14 in [1.0, 3.14, 2.0]", true);
}

test(cel_eval_in_with_parens_around_elements) {
    cel_expect_bool("2 in [(1), (2), (3)]", true);
}

test(cel_eval_in_with_expression_elements) {
    // List elements can be full expressions; each is evaluated before
    // membership check.
    cel_expect_bool("true in [1 == 1, 2 == 3]", true);
}

// Type-strict — element-wise equality respects kind
test(cel_eval_in_number_vs_string_list_all_mismatched_is_undefined) {
    // 1 == "1" → Undefined for every element; no match found.
    cel_expect_undefined("1 in [\"1\", \"2\", \"3\"]");
}

test(cel_eval_in_match_wins_over_type_mismatch) {
    // First element matches → true, regardless of later type mismatches.
    cel_expect_bool("1 in [1, \"x\"]", true);
}

test(cel_eval_in_match_after_type_mismatch) {
    // First element is a type mismatch (Undef), later match still wins.
    cel_expect_bool("1 in [\"x\", 1]", true);
}

test(cel_eval_in_no_match_with_some_mismatch_is_undefined) {
    // No element equals 1, but some comparisons are Undefined (1 vs
    // "x"). Undefined propagates.
    cel_expect_undefined("1 in [\"x\", 2, \"y\"]");
}

test(cel_eval_in_no_match_all_defined_is_false) {
    // No matches and every comparison is well-typed → false.
    cel_expect_bool("99 in [1, 2, 3]", false);
}

// Right operand must be a List
test(cel_eval_in_string_rhs_is_undefined) {
    // The old string-token semantics is gone. `"x" in "a b c"` is a
    // type error now — the right operand is a String, not a List.
    cel_expect_undefined("\"x\" in \"a b c\"");
}

test(cel_eval_in_number_rhs_is_undefined) {
    cel_expect_undefined("1 in 1");
}

test(cel_eval_in_bool_rhs_is_undefined) {
    cel_expect_undefined("\"a\" in true");
}

// Undefined propagation
test(cel_eval_in_undefined_lhs) {
    cel_test::Registry reg{};
    // Every comparison is `Undefined == elem` → Undefined → saw_undef →
    // result Undefined.
    cel_expect_undefined_in("missing in [1, 2, 3]", reg);
}

test(cel_eval_in_undefined_rhs) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("1 in missing", reg);
}

test(cel_eval_in_undefined_element) {
    // [missing] is a list of one Undefined value. 1 == Undefined →
    // Undefined → saw_undef → result Undefined.
    cel_test::Registry reg{};
    cel_expect_undefined_in("1 in [missing]", reg);
}

// Registry-backed: identifier resolves to a list value
test(cel_eval_in_identifier_resolves_to_list) {
    cel_test::Registry reg{{
        { "modes", seam::cel::Value::list({
            seam::cel::Value::string("fast"),
            seam::cel::Value::string("slow"),
        }) },
    }};
    cel_expect_bool_in("\"fast\" in modes", reg, true);
}

test(cel_eval_in_identifier_resolves_to_list_no_match) {
    cel_test::Registry reg{{
        { "modes", seam::cel::Value::list({
            seam::cel::Value::string("fast"),
            seam::cel::Value::string("slow"),
        }) },
    }};
    cel_expect_bool_in("\"medium\" in modes", reg, false);
}

test(cel_eval_in_left_identifier) {
    cel_test::Registry reg{{
        { "tag", seam::cel::Value::string("blue") },
    }};
    cel_expect_bool_in("tag in [\"red\", \"green\", \"blue\"]", reg, true);
}

// Realistic seam shape — what real modules emit
test(cel_eval_seam_idiom_channel_membership) {
    // After module-side template substitution this is what cel sees.
    cel_test::Registry reg{{
        { "enabled_channels", seam::cel::Value::list({
            seam::cel::Value::string("chan_a"),
            seam::cel::Value::string("chan_b"),
            seam::cel::Value::string("chan_c"),
        }) },
    }};
    cel_expect_bool_in("\"chan_a\" in enabled_channels", reg, true);
}

test(cel_eval_seam_idiom_channel_not_enabled) {
    cel_test::Registry reg{{
        { "enabled_channels", seam::cel::Value::list({
            seam::cel::Value::string("chan_a"),
            seam::cel::Value::string("chan_b"),
        }) },
    }};
    cel_expect_bool_in("\"chan_z\" in enabled_channels", reg, false);
}

// Word boundary — 'in' is still a keyword, identifiers like `index` /
// `inversion` aren't operators.
test(cel_compile_identifier_starting_with_in_still_works) {
    auto c = cel_test::compileSrc("index");
    assertTrue(c.ok);
    assertEqual("index", c.expr.references()[0].c_str());
}

test(cel_eval_in_keyword_against_identifier_named_index) {
    cel_test::Registry reg{{
        { "x",     seam::cel::Value::string("a") },
        { "index", seam::cel::Value::list({
            seam::cel::Value::string("a"),
            seam::cel::Value::string("b"),
        }) },
    }};
    cel_expect_bool_in("x in index", reg, true);
}

test(cel_compile_in_glued_to_identifier_fails) {
    // `[1, 2] inb` — 'in' isn't a keyword here (followed by 'b'); `inb`
    // is a fresh identifier sitting unexpectedly after the primary list.
    cel_expect_compile_error("[1, 2] inb", 7);     // 'i' of "inb"
}

// Combined with other operators
test(cel_eval_not_around_in) {
    cel_expect_bool("!(\"z\" in [\"x\", \"y\"])", true);
}

test(cel_eval_in_combined_with_and) {
    cel_test::Registry reg{{
        { "modes", seam::cel::Value::list({
            seam::cel::Value::string("fast"),
        }) },
    }};
    cel_expect_bool_in("\"fast\" in modes && true", reg, true);
}

test(cel_eval_or_with_in_pair) {
    cel_expect_bool("(1 in [1]) || (2 in [3])", true);
}

// References — tracks both identifier operands; literal lhs only refs rhs
test(cel_compile_in_tracks_both_identifier_operands) {
    auto c = cel_test::compileSrc("a in b");
    assertTrue(c.ok);
    assertEqual((size_t)2, c.expr.references().size());
}

test(cel_compile_in_with_literal_lhs_tracks_only_rhs) {
    auto c = cel_test::compileSrc("\"chan_a\" in enabled");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("enabled", c.expr.references()[0].c_str());
}

test(cel_compile_in_with_list_literal_tracks_list_elements) {
    auto c = cel_test::compileSrc("x in [a, b, c]");
    assertTrue(c.ok);
    // x, a, b, c — all referenced.
    assertEqual((size_t)4, c.expr.references().size());
}

// Non-chainable
test(cel_compile_eq_then_in_fails) {
    cel_expect_compile_error("a == b in [c]", 7);  // 'i' of "in"
}

test(cel_compile_in_then_eq_fails) {
    cel_expect_compile_error("a in [b] == c", 9);  // '=' of "=="
}

// Malformed
test(cel_compile_in_missing_rhs_fails) {
    cel_expect_compile_error("\"a\" in", 6);
}

test(cel_compile_in_at_start_is_identifier_then_trailing_fails) {
    // `in [1]` — 'in' parses as an identifier (no preceding primary);
    // then `[1]` is unexpected trailing content.
    cel_expect_compile_error("in [1]", 3);          // '[' of trailing list
}
