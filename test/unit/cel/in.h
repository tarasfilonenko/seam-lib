#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/in — string-token membership operator 'in'
//
// `item in container` checks whether `item` (string) appears as one of
// the whitespace-separated tokens in `container` (string). Matches the
// SEAM convention for `caps::Param::flags` and `options`.
//
// Type-strict: both operands must be String. Non-string operand →
// Undefined. Undefined operand → Undefined.
//
// 'in' is a keyword operator at the cmp_expr precedence level — same
// slot as ==, !=, <, etc. (non-chainable).
// ─────────────────────────────────────────────

#include "helpers.h"

// Happy path
test(cel_eval_in_single_token_match)   { cel_expect_bool("\"x\" in \"x\"",         true);  }
test(cel_eval_in_first_token_match)    { cel_expect_bool("\"x\" in \"x y z\"",     true);  }
test(cel_eval_in_middle_token_match)   { cel_expect_bool("\"y\" in \"x y z\"",     true);  }
test(cel_eval_in_last_token_match)     { cel_expect_bool("\"z\" in \"x y z\"",     true);  }
test(cel_eval_in_no_match)             { cel_expect_bool("\"q\" in \"x y z\"",     false); }
test(cel_eval_in_empty_container)      { cel_expect_bool("\"x\" in \"\"",          false); }
test(cel_eval_in_empty_item)           { cel_expect_bool("\"\"  in \"x y\"",       false); }

test(cel_eval_in_extra_whitespace_in_container) {
    // Multiple spaces / tabs / newlines between tokens are all separators.
    cel_expect_bool("\"x\" in \"  x  y  \"", true);
}

test(cel_eval_in_tab_separator)  { cel_expect_bool("\"y\" in \"x\\ty\\tz\"", true); }
test(cel_eval_in_newline_separator) {
    // CEL source string body contains a literal LF — that's allowed by
    // the string grammar and tokenises correctly.
    cel_expect_bool("\"y\" in \"x\\ny\\nz\"", true);
}

test(cel_eval_in_multi_token_item_no_match) {
    // Item is "x y" — not a single token, so doesn't match either "x" or
    // "y" alone.
    cel_expect_bool("\"x y\" in \"x y z\"", false);
}

// Identifier-backed (the realistic shape)
test(cel_eval_in_with_registry_match) {
    cel_test::Registry reg{{
        { "enabled_channels", seam::cel::Value::string("chan_a chan_b chan_c") },
    }};
    cel_expect_bool_in("\"chan_a\" in enabled_channels", reg, true);
}

test(cel_eval_in_with_registry_miss) {
    cel_test::Registry reg{{
        { "enabled_channels", seam::cel::Value::string("chan_a chan_b chan_c") },
    }};
    cel_expect_bool_in("\"chan_z\" in enabled_channels", reg, false);
}

test(cel_eval_in_both_identifiers) {
    cel_test::Registry reg{{
        { "tag",  seam::cel::Value::string("blue")  },
        { "tags", seam::cel::Value::string("red green blue") },
    }};
    cel_expect_bool_in("tag in tags", reg, true);
}

// Type-strict — non-string operand → Undefined
test(cel_eval_in_number_lhs_is_undefined) { cel_expect_undefined("1 in \"1 2 3\""); }
test(cel_eval_in_number_rhs_is_undefined) { cel_expect_undefined("\"a\" in 5");      }
test(cel_eval_in_bool_lhs_is_undefined)   { cel_expect_undefined("true in \"true false\""); }
test(cel_eval_in_bool_rhs_is_undefined)   { cel_expect_undefined("\"a\" in true");   }
test(cel_eval_in_number_both_is_undefined){ cel_expect_undefined("1 in 1");          }

// Undefined propagation
test(cel_eval_in_undefined_lhs_is_undefined) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing in \"x y\"", reg);
}

test(cel_eval_in_undefined_rhs_is_undefined) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("\"x\" in missing", reg);
}

// Word boundary — 'in' is only a keyword when not followed by [A-Za-z0-9_]
test(cel_compile_identifier_starting_with_in_succeeds) {
    // `index` is a plain identifier, not `in` + `dex`.
    auto c = cel_test::compileSrc("index");
    assertTrue(c.ok);
    assertEqual("index", c.expr.references()[0].c_str());
}

test(cel_eval_in_keyword_against_identifier_like_rhs) {
    // `x in index` — `index` is the rhs identifier; `in` is the operator.
    cel_test::Registry reg{{
        { "x",     seam::cel::Value::string("a") },
        { "index", seam::cel::Value::string("a b c") },
    }};
    cel_expect_bool_in("x in index", reg, true);
}

test(cel_compile_in_glued_to_identifier_fails) {
    // `"a" inb` — `in` is not a keyword here because it's followed by a
    // word-continuation char. `inb` is therefore a fresh identifier, but
    // it sits unexpectedly after the primary `"a"`.
    cel_expect_compile_error("\"a\" inb", 4);   // 'i' of "inb"
}

test(cel_compile_in_followed_by_underscore_fails) {
    cel_expect_compile_error("\"a\" in_thing", 4);
}

test(cel_compile_in_followed_by_digit_fails) {
    cel_expect_compile_error("\"a\" in9", 4);
}

// Combined with other operators
test(cel_eval_in_combined_with_and) {
    cel_test::Registry reg{{
        { "tags", seam::cel::Value::string("blue green") },
    }};
    cel_expect_bool_in("\"blue\" in tags && true", reg, true);
}

test(cel_eval_not_around_in) {
    // !("z" in "x y") = !false = true
    cel_expect_bool("!(\"z\" in \"x y\")", true);
}

test(cel_eval_or_with_in) {
    cel_expect_bool("\"x\" in \"a b\" || \"x\" in \"x y\"", true);
}

// References — tracks both operands when they're identifiers
test(cel_compile_in_tracks_both_identifier_operands) {
    auto c = cel_test::compileSrc("a in b");
    assertTrue(c.ok);
    assertEqual((size_t)2, c.expr.references().size());
    assertEqual("a", c.expr.references()[0].c_str());
    assertEqual("b", c.expr.references()[1].c_str());
}

test(cel_compile_in_with_literal_lhs_tracks_only_rhs) {
    auto c = cel_test::compileSrc("\"chan_a\" in enabled");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("enabled", c.expr.references()[0].c_str());
}

// Non-chainable with other cmp ops
test(cel_compile_eq_then_in_fails) {
    // `a == b in c` — second cmp op is unexpected (cmp_expr is non-chainable).
    cel_expect_compile_error("a == b in c", 7);  // 'i' of "in"
}

test(cel_compile_in_then_eq_fails) {
    cel_expect_compile_error("a in b == c", 7);  // '=' of "=="
}

// Malformed
test(cel_compile_in_missing_rhs_fails) {
    cel_expect_compile_error("\"a\" in", 6);     // EOF after 'in'
}

test(cel_compile_in_lhs_only_keyword_position) {
    // `in "x y"` — `in` at the start is an identifier (no preceding primary
    // for it to operate on). Parses as Identifier(in), then trailing `"x y"`
    // errors.
    cel_expect_compile_error("in \"x y\"", 3);   // '"' of trailing string
}
