#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/not — unary logical negation '!'
//
// not_expr := "!" not_expr | primary
//
// Type-strict: ! requires bool. Anything else (number, string, etc.)
// or Undefined produces Undefined.
// ─────────────────────────────────────────────

#include "helpers.h"

// Happy path — bool inversion
test(cel_eval_not_true)         { cel_expect_bool("!true",   false); }
test(cel_eval_not_false)        { cel_expect_bool("!false",  true);  }
test(cel_eval_double_not_true)  { cel_expect_bool("!!true",  true);  }
test(cel_eval_double_not_false) { cel_expect_bool("!!false", false); }
test(cel_eval_triple_not)       { cel_expect_bool("!!!true", false); }

test(cel_eval_not_with_parens)        { cel_expect_bool("!(true)",  false); }
test(cel_eval_not_with_parens_false)  { cel_expect_bool("!(false)", true);  }

test(cel_eval_paren_wraps_not)        { cel_expect_bool("(!true)",  false); }
test(cel_eval_not_of_paren_not)       { cel_expect_bool("!(!true)", true);  }

// Whitespace
test(cel_eval_not_with_whitespace)        { cel_expect_bool("!  true",  false); }
test(cel_eval_double_not_with_whitespace) { cel_expect_bool("! ! true", true);  }

// Identifier resolution + propagation
test(cel_eval_not_bool_identifier) {
    cel_test::Registry reg{{ { "muted", seam::cel::Value::boolean(true) } }};
    cel_expect_bool_in("!muted", reg, false);
}

test(cel_eval_double_not_bool_identifier) {
    cel_test::Registry reg{{ { "muted", seam::cel::Value::boolean(true) } }};
    cel_expect_bool_in("!!muted", reg, true);
}

test(cel_eval_not_unbound_identifier_is_undefined) {
    // Operand is Undefined → result is Undefined (propagation).
    cel_test::Registry reg{};
    cel_expect_undefined_in("!missing", reg);
}

test(cel_eval_not_undefined_in_parens_is_undefined) {
    cel_test::Registry reg{};
    cel_expect_undefined_in("!(missing)", reg);
}

// Type-strict: ! requires bool
test(cel_eval_not_number_is_undefined) { cel_expect_undefined("!42");    }
test(cel_eval_not_string_is_undefined) { cel_expect_undefined("!\"hi\""); }
test(cel_eval_not_zero_is_undefined) {
    // Even though many languages treat 0 as falsy, cel is type-strict —
    // !0 is Undefined, not true.
    cel_expect_undefined("!0");
}

// References — operand still tracked through Not
test(cel_compile_not_tracks_operand_reference) {
    auto c = cel_test::compileSrc("!foo");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("foo", c.expr.references()[0].c_str());
}

test(cel_compile_double_not_tracks_reference_once) {
    // De-dup: the same identifier inside nested Nots only shows up once.
    auto c = cel_test::compileSrc("!!foo");
    assertTrue(c.ok);
    assertEqual((size_t)1, c.expr.references().size());
    assertEqual("foo", c.expr.references()[0].c_str());
}

// Malformed
test(cel_compile_lone_not_fails) {
    // `!` with nothing after → parsePrimary fails at EOF.
    cel_expect_compile_error("!", 1);
}

test(cel_compile_double_not_lone_fails) {
    cel_expect_compile_error("!!", 2);
}

test(cel_compile_not_then_at_alone_fails) {
    // `!@` — '@' isn't a valid bare_id start by itself.
    cel_expect_compile_error("!@", 1);
}

test(cel_compile_not_then_unterminated_string_fails) {
    // Error propagates from the inner parseString.
    cel_expect_compile_error("!\"oops", 1);   // opening quote of inner string
}
