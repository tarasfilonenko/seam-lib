#pragma once
// ─────────────────────────────────────────────
// seam::protocol::caps::Action
//
// Models an ACTION block from a SEAM CAPS response.
// Describes an invokable action and its arguments.
// No values — see seam::protocol::wire::Command for data exchange.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include "Arg.h"

namespace seam {
namespace protocol {
namespace caps {

struct Action {
    std::string      id;
    std::string      label;
    std::string      description;
    std::string      enabled_expr;   // CEL expression, empty = always enabled
    std::string      visible_expr;   // CEL expression, empty = always visible
    std::string      trigger;        // param id that drives this action, empty if none
    std::vector<Arg> args;
};

} // namespace caps
} // namespace protocol
} // namespace seam