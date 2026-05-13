#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/identifier — bare + @-prefixed identifiers
//
// Grammar:
//   identifier := bare_id | "@" bare_id
//   bare_id    := /[A-Za-z_][A-Za-z0-9_]*/
//
// Bare ids resolve to caps param ids; @-prefixed are reserved for
// host-provided state. The '@' is part of the name passed to
// Env::resolve, so bare and @-prefixed live in disjoint namespaces.
// ─────────────────────────────────────────────

#include "helpers.h"

// Bare identifiers — compile + references
test(cel_compile_bare_identifier_succeeds) {
    auto c = cel_test::compileSrc("foo");
    assertTrue(c.ok);
    assertFalse(c.expr.isAlwaysTrue());
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("foo", c.expr.references()[0].c_str());
}

test(cel_compile_underscore_prefix_identifier_succeeds) {
    auto c = cel_test::compileSrc("_internal");
    assertTrue(c.ok);
    assertEqual("_internal", c.expr.references()[0].c_str());
}

test(cel_compile_identifier_with_digits_succeeds) {
    // bare_id := [A-Za-z_][A-Za-z0-9_]*  — digits allowed after first char
    auto c = cel_test::compileSrc("chan_1");
    assertTrue(c.ok);
    assertEqual("chan_1", c.expr.references()[0].c_str());
}

test(cel_compile_capitalised_keyword_is_identifier) {
    // "true"/"false" are case-sensitive keywords; "True"/"TRUE" are plain
    // identifiers that resolve via Env.
    auto c = cel_test::compileSrc("TRUE");
    assertTrue(c.ok);
    assertEqual("TRUE", c.expr.references()[0].c_str());
}

// Bare identifiers — evaluation
test(cel_evaluate_bare_identifier_resolves_via_env) {
    cel_test::Registry reg{{ { "gain", seam::cel::Value::number(3.5) } }};
    cel_expect_number_in("gain", reg, 3.5);
}

test(cel_evaluate_identifier_returns_bool_value) {
    cel_test::Registry reg{{ { "muted", seam::cel::Value::boolean(true) } }};
    cel_expect_bool_in("muted", reg, true);
}

test(cel_evaluate_identifier_returns_string_value) {
    cel_test::Registry reg{{ { "mode", seam::cel::Value::string("advanced") } }};
    cel_expect_string_in("mode", reg, "advanced");
}

test(cel_evaluate_unbound_identifier_is_undefined) {
    // Env doesn't know the identifier → Undefined propagates. Hosts
    // treat Undefined as "not ready" (e.g. hide the row until a real
    // value arrives).
    cel_test::Registry reg{};
    cel_expect_undefined_in("missing", reg);
}

// @-prefixed identifiers
test(cel_compile_at_prefixed_identifier_succeeds) {
    auto c = cel_test::compileSrc("@connected");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("@connected", c.expr.references()[0].c_str());   // '@' is part of the name
}

test(cel_evaluate_at_prefixed_identifier_resolves) {
    cel_test::Registry reg{{ { "@connected", seam::cel::Value::boolean(true) } }};
    cel_expect_bool_in("@connected", reg, true);
}

test(cel_evaluate_at_and_bare_are_distinct_namespaces) {
    // The '@' prefix is part of the identifier name passed to Env; the
    // resolver sees a different string and can route to host vs param
    // state. Reserve discipline: a bare "foo" and "@foo" don't collide.
    cel_test::Registry reg{{
        { "foo",  seam::cel::Value::number(1.0) },
        { "@foo", seam::cel::Value::number(2.0) },
    }};
    cel_expect_number_in("foo",  reg, 1.0);
    cel_expect_number_in("@foo", reg, 2.0);
}

test(cel_compile_at_underscore_prefix_succeeds) {
    auto c = cel_test::compileSrc("@_internal");
    assertTrue(c.ok);
    assertEqual("@_internal", c.expr.references()[0].c_str());
}

// Identifiers — malformed
test(cel_compile_lone_at_fails)               { cel_expect_compile_error("@",     0); }
test(cel_compile_at_followed_by_at_fails)     { cel_expect_compile_error("@@foo", 0); }

test(cel_compile_at_followed_by_digit_fails) {
    // After '@' we need a bare_id which must start with a letter or '_'.
    // Digits are not valid bare_id starts.
    cel_expect_compile_error("@0bar", 0);
}

test(cel_compile_identifier_then_garbage_fails)    { cel_expect_compile_error("foo bar",  4); }
test(cel_compile_at_identifier_then_garbage_fails) { cel_expect_compile_error("@foo bar", 5); }

// References — populated on success, cleared on failure
test(cel_compile_failure_leaves_references_empty) {
    auto c = cel_test::compileSrc("foo bar");       // fails after parsing "foo"
    assertFalse(c.ok);
    assertEqual((size_t)0, c.expr.references().size());
    assertTrue(c.expr.isAlwaysTrue());
}

test(cel_compile_empty_source_has_no_references) {
    auto c = cel_test::compileSrc("");
    assertEqual((size_t)0, c.expr.references().size());
}

test(cel_compile_bool_literal_has_no_references) {
    auto c = cel_test::compileSrc("true");
    assertEqual((size_t)0, c.expr.references().size());
}
