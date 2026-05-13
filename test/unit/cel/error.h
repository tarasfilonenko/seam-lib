#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/error — error reporting (line/column, paired-opener,
// formatError)
//
// Covers the post-step-9 polish:
//   - CompileError now carries line + column (1-based) computed from
//     position.
//   - CompileError carries paired_position for delimiter errors so
//     callers can see both the error point and the matching opener.
//   - seam::cel::formatError renders all of this as a human-readable
//     multi-line string with a caret under the error column.
// ─────────────────────────────────────────────

#include "helpers.h"

// ── line / column ─────────────────────────────

test(cel_error_line_column_for_single_line_source) {
    auto c = cel_test::compileSrc("1 = 2");
    assertFalse(c.ok);
    assertEqual((size_t)2, c.err.position);
    assertEqual((size_t)1, c.err.line);
    assertEqual((size_t)3, c.err.column);    // column is 1-based: pos 2 → col 3
}

test(cel_error_line_column_for_multi_line_source) {
    // Source contains a literal newline byte inside a string. Error is
    // on the second line at column 1 (the '@' that follows the newline).
    // Source: '\n@' (newline then '@').
    auto c = cel_test::compileSrc("\n@");
    assertFalse(c.ok);
    assertEqual((size_t)1, c.err.position);  // the '@' is at byte 1
    assertEqual((size_t)2, c.err.line);
    assertEqual((size_t)1, c.err.column);
}

test(cel_error_line_column_zero_when_no_error) {
    auto c = cel_test::compileSrc("true");
    assertTrue(c.ok);
    assertEqual((size_t)0, c.err.line);      // unset → 0
    assertEqual((size_t)0, c.err.column);
}

// ── paired-opener info ────────────────────────

test(cel_error_paired_set_for_unclosed_paren) {
    auto c = cel_test::compileSrc("(true");
    assertFalse(c.ok);
    assertTrue(c.err.has_paired);
    assertEqual((size_t)0, c.err.paired_position);   // '(' is at offset 0
    assertEqual((size_t)0, c.err.position);          // error reported at the opener
}

test(cel_error_paired_set_for_expected_close_paren) {
    auto c = cel_test::compileSrc("(true xyz)");
    assertFalse(c.ok);
    assertTrue(c.err.has_paired);
    assertEqual((size_t)0, c.err.paired_position);   // '(' at offset 0
    assertEqual((size_t)6, c.err.position);          // 'x' at offset 6
}

test(cel_error_paired_set_for_unclosed_list) {
    auto c = cel_test::compileSrc("[1, 2");
    assertFalse(c.ok);
    assertTrue(c.err.has_paired);
    assertEqual((size_t)0, c.err.paired_position);
}

test(cel_error_paired_clear_for_non_delimiter_error) {
    // Non-delimiter errors should leave has_paired false.
    auto c = cel_test::compileSrc("1 = 2");
    assertFalse(c.ok);
    assertFalse(c.err.has_paired);
}

// ── formatError ──────────────────────────────

test(cel_format_returns_empty_for_no_error) {
    seam::cel::CompileError err{};
    std::string out = seam::cel::formatError("anything", err);
    assertEqual((size_t)0, out.size());
}

test(cel_format_includes_line_column_and_message) {
    auto c = cel_test::compileSrc("1 = 2");
    std::string out = seam::cel::formatError("1 = 2", c.err);
    // Output contains the line/col header. We check substrings rather
    // than exact text to keep the test resilient to minor wording.
    assertTrue(out.find("line 1") != std::string::npos);
    assertTrue(out.find("column 3") != std::string::npos);
    assertTrue(out.find("unexpected trailing content") != std::string::npos);
}

test(cel_format_includes_source_line_and_caret) {
    auto c = cel_test::compileSrc("1 = 2");
    std::string out = seam::cel::formatError("1 = 2", c.err);
    assertTrue(out.find("1 = 2") != std::string::npos);    // the source line
    assertTrue(out.find("^") != std::string::npos);         // a caret somewhere
}

test(cel_format_includes_paired_opener_pointer) {
    auto c = cel_test::compileSrc("(true xyz)");
    std::string out = seam::cel::formatError("(true xyz)", c.err);
    assertTrue(out.find("matching opener") != std::string::npos);
}

test(cel_format_omits_paired_pointer_for_non_paired_errors) {
    auto c = cel_test::compileSrc("1 = 2");
    std::string out = seam::cel::formatError("1 = 2", c.err);
    assertTrue(out.find("matching opener") == std::string::npos);
}
