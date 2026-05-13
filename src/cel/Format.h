#pragma once
// ─────────────────────────────────────────────
// seam::cel::formatError
//
// Pretty-printer for CompileError. Produces a multi-line string of the
// form:
//
//     error at line 1, column 8: expected ')' to close '('
//       gain > (5
//              ^   here
//              ↑ matching '(' here  (when paired info is present)
//
// Useful for host logs that need a human-readable diagnostic. The
// underlying CompileError fields (`position`, `line`, `column`,
// `paired_position`) remain available for programmatic handling — this
// helper is purely a convenience layer on top.
//
// Caller policy is unchanged: on a compile error, the host treats the
// source as "always true" (fail-open) and logs whatever it likes —
// typically formatError() output plus the original source.
// ─────────────────────────────────────────────

#include <cstddef>
#include <string>
#include <string_view>

#include "Compile.h"   // CompileError

namespace seam {
namespace cel {

namespace detail {

// Span of [start, end) covering the line that contains `position`.
inline void findLineBounds(std::string_view source, size_t position,
                           size_t &out_start, size_t &out_end) {
    if (position > source.size()) position = source.size();
    out_start = 0;
    for (size_t i = 0; i < position; ++i) {
        if (source[i] == '\n') out_start = i + 1;
    }
    out_end = out_start;
    while (out_end < source.size() && source[out_end] != '\n') ++out_end;
}

// Builds a caret prefix that aligns under `position` within a line that
// starts at `line_start`. Tabs in the source are preserved as tabs so
// the caret lines up correctly when the snippet is rendered.
inline std::string buildCaretPrefix(std::string_view source,
                                    size_t line_start, size_t position) {
    std::string out;
    if (position > source.size()) position = source.size();
    for (size_t i = line_start; i < position; ++i) {
        out.push_back(source[i] == '\t' ? '\t' : ' ');
    }
    return out;
}

} // namespace detail

// Renders the error as a human-readable, multi-line string. Returns an
// empty string if `err.message` is empty (i.e. no error). The output
// always ends with a newline.
inline std::string formatError(std::string_view source, const CompileError &err) {
    if (err.message.empty()) return {};

    std::string out;
    out += "error at line ";
    out += std::to_string(err.line);
    out += ", column ";
    out += std::to_string(err.column);
    out += ": ";
    out += err.message;
    out += '\n';

    size_t line_start = 0;
    size_t line_end   = 0;
    detail::findLineBounds(source, err.position, line_start, line_end);

    out += "  ";
    out += source.substr(line_start, line_end - line_start);
    out += '\n';
    out += "  ";
    out += detail::buildCaretPrefix(source, line_start, err.position);
    out += "^\n";

    // When the error carries a paired opener (`(` for `expected ')'`,
    // `[` for `unclosed '['`, etc.), include a second caret pointing at
    // the opener — but only if it's on the same line as the error.
    // Cross-line pairing would need a more elaborate renderer.
    if (err.has_paired) {
        size_t pair_line_start = 0;
        size_t pair_line_end   = 0;
        detail::findLineBounds(source, err.paired_position, pair_line_start, pair_line_end);
        if (pair_line_start == line_start) {
            out += "  ";
            out += detail::buildCaretPrefix(source, line_start, err.paired_position);
            out += "↑ matching opener here\n";
        }
    }

    return out;
}

} // namespace cel
} // namespace seam
