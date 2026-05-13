#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/number — number literals
//
// Grammar:  number := /-? [0-9]+ ( "." [0-9]+ )?/
// No scientific notation, no '+' sign, no leading dot.
// ─────────────────────────────────────────────

#include "helpers.h"

// Happy path
test(cel_compile_integer_zero)            { cel_expect_number("0",   0.0); }
test(cel_compile_positive_integer)        { cel_expect_number("42", 42.0); }
test(cel_compile_negative_integer)        { cel_expect_number("-7", -7.0); }
test(cel_compile_decimal)                 { cel_expect_number("3.14", 3.14); }
test(cel_compile_negative_decimal)        { cel_expect_number("-2.5", -2.5); }
test(cel_compile_number_with_whitespace)  { cel_expect_number("  100  ", 100.0); }

test(cel_compile_negative_zero_evaluates_to_zero) {
    // -0 is a valid number literal; IEEE negative zero compares equal to
    // positive zero numerically, which is what we care about here.
    cel_expect_number("-0", 0.0);
}

test(cel_compile_leading_zeros_are_decimal) {
    // Not octal — `007` is just the number 7. This matches strtod's
    // behaviour and avoids surprising module authors who might write
    // padded values.
    cel_expect_number("007", 7.0);
}

// Malformed
test(cel_compile_trailing_dot_fails)        { cel_expect_compile_error("1.",      1); }
test(cel_compile_lone_minus_fails)          { cel_expect_compile_error("-",       0); }
test(cel_compile_leading_dot_fails)         { cel_expect_compile_error(".5",      0); }
test(cel_compile_number_then_garbage_fails) { cel_expect_compile_error("42 xyz",  3); }

test(cel_compile_double_minus_fails) {
    // `--5` — there is no unary minus operator in the grammar; the leading
    // sign is part of the number literal token, so `-` followed by `-` is
    // not a number start and there's no other primary that begins with `-`.
    cel_expect_compile_error("--5", 0);
}

test(cel_compile_positive_sign_fails) {
    // `+5` — the grammar permits only an optional leading `-`. `+` is not
    // a primary start at all.
    cel_expect_compile_error("+5", 0);
}

test(cel_compile_minus_then_plus_fails) { cel_expect_compile_error("-+5", 0); }

test(cel_compile_double_dot_fails) {
    // `1.5.5` — first `1.5` parses cleanly as a decimal, then the trailing
    // `.5` is unexpected content after the primary.
    cel_expect_compile_error("1.5.5", 3);   // second '.'
}

test(cel_compile_consecutive_dots_fails) {
    // `1..5` — after the first `.` the lexer demands at least one digit
    // before exiting the fractional part. Error points at the first dot
    // because that's where the malformed number begins.
    cel_expect_compile_error("1..5", 1);    // first '.'
}

test(cel_compile_scientific_notation_not_supported) {
    // `1e5` is not in the grammar. The number lexer consumes `1`, then
    // `e5` is trailing content. We don't try to recognise scientific
    // notation — caps values that need it should be expressed differently.
    cel_expect_compile_error("1e5", 1);
}

test(cel_compile_number_glued_to_identifier_fails) {
    // `123abc` — number lexer stops at `a`; the trailing word is not a
    // valid continuation of a primary.
    cel_expect_compile_error("123abc", 3);
}
