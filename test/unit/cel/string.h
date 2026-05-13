#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/string — string literals
//
// Grammar:  string := /"([^"\\]|\\.)*"/
// Supported escapes: \" \\ \n \t \r
// ─────────────────────────────────────────────

#include "helpers.h"

// Happy path
test(cel_compile_empty_string)        { cel_expect_string("\"\"",     ""); }
test(cel_compile_simple_string)       { cel_expect_string("\"hello\"", "hello"); }
test(cel_compile_string_with_spaces)  { cel_expect_string("\"a b c\"", "a b c"); }

test(cel_compile_string_with_newline_escape) {
    // CEL source:  "line\nbreak"   ← \n is a CEL escape that decodes to LF
    cel_expect_string("\"line\\nbreak\"", "line\nbreak");
}

test(cel_compile_string_with_tab_and_cr_escapes) {
    cel_expect_string("\"a\\tb\\rc\"", "a\tb\rc");
}

test(cel_compile_string_with_escaped_quote) {
    // CEL source:  "say \"hi\""
    cel_expect_string("\"say \\\"hi\\\"\"", "say \"hi\"");
}

test(cel_compile_string_with_escaped_backslash) {
    // CEL source:  "back\\slash"
    cel_expect_string("\"back\\\\slash\"", "back\\slash");
}

test(cel_compile_string_with_literal_newline_byte) {
    // The grammar's char class is `[^"\\]`, so a raw newline byte inside
    // a string is permitted and passes through unchanged. (Only an
    // unescaped quote or backslash terminate the body.)
    cel_expect_string("\"a\nb\"", "a\nb");
}

// Malformed
test(cel_compile_unterminated_string_fails) {
    cel_expect_compile_error("\"hello",         0);     // opening quote
}

test(cel_compile_bad_escape_fails) {
    // "oops\xthing" — bad escape char 'x' at index 6
    cel_expect_compile_error("\"oops\\xthing\"", 6);
}

test(cel_compile_trailing_backslash_fails) {
    // String ends with `\` and no following char → unterminated
    cel_expect_compile_error("\"oops\\",        0);
}

test(cel_compile_string_with_zero_escape_fails) {
    // `\0` is not in the supported escape set (\", \\, \n, \t, \r).
    cel_expect_compile_error("\"x\\0y\"",       3);     // the '0' after '\'
}
