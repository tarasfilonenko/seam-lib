#pragma once
// ─────────────────────────────────────────────
// seam::protocol::caps::Group
//
// Models a GROUP block from a SEAM CAPS response.
// Organises params, actions and streams into logical sections.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include "Param.h"
#include "Action.h"
#include "Stream.h"

namespace seam {
namespace protocol {
namespace caps {

struct Group {
    std::string          id;
    std::string          label;
    std::string          enabled_expr;   // CEL expression, empty = always enabled
    std::string          visible_expr;   // CEL expression, empty = always visible
    std::vector<Param>   params;
    std::vector<Action>  actions;
    std::vector<Stream>  streams;
};

} // namespace caps
} // namespace protocol
} // namespace seam