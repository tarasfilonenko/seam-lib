#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/list — list literals
//
// list_lit := "[" ( expr ( "," expr )* )? "]"
//
// Empty list `[]` is allowed. Trailing commas are NOT (`[1,]` errors).
// Elements are full expressions, so list literals nest naturally.
//
// List values aren't otherwise comparable today (no `==` between lists,
// no `<`, no `!`). Lists exist primarily to feed the `in` operator —
// see in.h for membership semantics.
// ─────────────────────────────────────────────

#include "helpers.h"

// Compile success — happy path
test(cel_compile_empty_list) {
    auto c = cel_test::compileSrc("[]");
    assertTrue(c.ok);
    assertFalse(c.expr.isAlwaysTrue());
}

test(cel_compile_singleton_list)    { cel_expect_compile_ok("[1]"); }
test(cel_compile_three_int_list)    { cel_expect_compile_ok("[1, 2, 3]"); }
test(cel_compile_string_list)       { cel_expect_compile_ok("[\"a\", \"b\"]"); }
test(cel_compile_bool_list)         { cel_expect_compile_ok("[true, false, true]"); }
test(cel_compile_mixed_type_list) {
    // Heterogeneous lists are permitted at parse time. Whether they're
    // useful depends on what they're compared against — element-wise
    // equality is type-strict.
    cel_expect_compile_ok("[1, \"a\", true]");
}

test(cel_compile_list_with_identifier_elements) {
    auto c = cel_test::compileSrc("[gain, mode, streaming]");
    assertTrue(c.ok);
    assertEqual((size_t)3, c.expr.references().size());
}

test(cel_compile_list_with_expression_elements) {
    // Elements are full expressions: comparisons, parenthesised primaries.
    cel_expect_compile_ok("[1 == 1, 2 == 3, (true)]");
}

test(cel_compile_nested_list) {
    cel_expect_compile_ok("[[1, 2], [3, 4]]");
}

test(cel_compile_list_with_whitespace) {
    cel_expect_compile_ok("[  1  ,  2  ,  3  ]");
}

test(cel_compile_list_with_newlines) {
    cel_expect_compile_ok("[\n  1,\n  2,\n  3\n]");
}

// Compile failures
test(cel_compile_unclosed_list_at_eof_fails) {
    cel_expect_compile_error("[1, 2", 0);          // opener offset
}

test(cel_compile_trailing_comma_fails) {
    // `[1, 2,]` — after the second ',', parser expects an expression but
    // sees ']'. parsePrimary fails at the ']' offset.
    cel_expect_compile_error("[1, 2,]", 6);
}

test(cel_compile_lone_comma_fails) {
    cel_expect_compile_error("[,]", 1);            // ',' isn't a primary start
}

test(cel_compile_missing_comma_fails) {
    // `[1 2]` — after parsing 1, parser expects ',' or ']' but finds '2'.
    cel_expect_compile_error("[1 2]", 3);
}

test(cel_compile_unmatched_close_bracket_fails) {
    cel_expect_compile_error("1]", 1);             // ']' is trailing content
}

test(cel_compile_lone_open_bracket_fails) {
    cel_expect_compile_error("[", 1);              // EOF where primary expected
}

test(cel_compile_lone_close_bracket_fails) {
    cel_expect_compile_error("]", 0);              // ']' isn't a primary
}

// Evaluation — list values exist but aren't directly testable through
// boolean/number/string. We cover them via 'in' tests in in.h; here we
// just confirm a list expression evaluates without errors and is
// recognised as a List.
test(cel_eval_empty_list_is_list_kind) {
    seam::cel::Value v = cel_test::evalEmpty("[]");
    assertTrue(v.isList());
    auto data = v.asList();
    assertTrue(data != nullptr);
    assertEqual((size_t)0, data->items.size());
}

test(cel_eval_int_list_contents) {
    seam::cel::Value v = cel_test::evalEmpty("[1, 2, 3]");
    assertTrue(v.isList());
    auto data = v.asList();
    assertEqual((size_t)3, data->items.size());
    assertTrue(data->items[0].isNumber());
    assertNear(1.0, data->items[0].asNumber(), 1e-9);
    assertNear(2.0, data->items[1].asNumber(), 1e-9);
    assertNear(3.0, data->items[2].asNumber(), 1e-9);
}

test(cel_eval_nested_list) {
    seam::cel::Value v = cel_test::evalEmpty("[[1], [2, 3]]");
    assertTrue(v.isList());
    auto outer = v.asList();
    assertEqual((size_t)2, outer->items.size());
    assertTrue(outer->items[0].isList());
    assertTrue(outer->items[1].isList());
    assertEqual((size_t)1, outer->items[0].asList()->items.size());
    assertEqual((size_t)2, outer->items[1].asList()->items.size());
}

test(cel_eval_list_element_undefined_propagates_via_in) {
    // [missing] doesn't error at list construction time — the list
    // simply contains an Undefined element. It only matters when the
    // list participates in 'in' comparison (see in.h tests).
    cel_test::Registry reg{};
    seam::cel::Env env = cel_test::envFor(reg);
    seam::cel::Value v = cel_test::eval("[missing]", env);
    assertTrue(v.isList());
    auto data = v.asList();
    assertEqual((size_t)1, data->items.size());
    assertTrue(data->items[0].isUndefined());
}

// List as primary in other contexts — type errors that aren't usefully
// supported today. Documented as expected behaviour.
test(cel_eval_list_eq_list_is_undefined) {
    // No element-wise list equality; lists pass through to evalCmp's
    // default Undefined.
    cel_expect_undefined("[1] == [1]");
}

test(cel_eval_not_list_is_undefined) {
    // '!' requires bool.
    cel_expect_undefined("![]");
}

test(cel_eval_list_lt_list_is_undefined) {
    // Ordering requires Number.
    cel_expect_undefined("[1] < [2]");
}

test(cel_eval_list_and_bool_is_undefined) {
    // Logical ops require bool; a non-bool / non-absorbing op is Undef.
    cel_expect_undefined("[1] && true");
}
