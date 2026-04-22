#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Group
//
// Models a GROUP block from a SEAM CAPS response.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include "Param.h"
#include "Action.h"
#include "Stream.h"

namespace seam {
namespace protocol {

struct Group {
    std::string          id;
    std::string          label;
    std::string          enabled_expr;  // CEL expression, empty = always enabled
    std::vector<Param>   params;
    std::vector<Action>  actions;
    std::vector<Stream>  streams;
};

} // namespace protocol
} // namespace seam