#pragma once
// ─────────────────────────────────────────────
// seam::cel::Env
//
// Identifier resolution surface for the SEAM-CEL evaluator.
// The evaluator never knows what identifiers mean — it asks the Env to
// look them up. This keeps the evaluator independent of any host model
// (e.g. ModuleModel's param store) and easy to test in isolation.
//
// Identifiers come in two flavours:
//   - bare identifiers     → caps param ids ("gain", "mode", "streaming")
//   - '@'-prefixed names   → reserved for future host-provided state
//                             ("@connected", "@preset_index", ...)
//
// Callback style is plain function pointer + opaque context to match the
// rest of the codebase (e.g. ui::layout::UiNode handlers) and to keep the
// evaluator allocation-free.
//
// Return Value::undefined() for identifiers that aren't bound — the
// evaluator propagates Undefined through any expression that references
// them, which is how "values not yet loaded" become "not ready".
// ─────────────────────────────────────────────

#include <string_view>

#include "Value.h"

namespace seam {
namespace cel {

struct Env {
    using LookupFn = Value (*)(void *ctx, std::string_view identifier);

    void     *ctx    = nullptr;
    LookupFn  lookup = nullptr;

    Value resolve(std::string_view identifier) const {
        return lookup ? lookup(ctx, identifier) : Value::undefined();
    }
};

} // namespace cel
} // namespace seam
