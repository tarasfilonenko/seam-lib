#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Action
//
// Models an ACTION block from a SEAM CAPS response.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include "Arg.h"

namespace seam {
namespace protocol {

struct Action {
    std::string      id;
    std::string      label;
    std::string      description;
    std::string      enabled_expr;  // CEL expression, empty = always enabled
    std::vector<Arg> args;
};

} // namespace protocol
} // namespace seam