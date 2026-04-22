#pragma once
// ─────────────────────────────────────────────
// seam::protocol::caps::Param
//
// Models a PARAM block from a SEAM CAPS response.
// Describes a parameter's type, access, and constraints.
// No values — see seam::protocol::wire for data exchange.
// ─────────────────────────────────────────────

#include <string>

namespace seam {
namespace protocol {
namespace caps {

struct Param {
    std::string id;
    std::string label;
    std::string description;
    std::string type;           // MIME type e.g. seam/int, image/png
    char        access;         // 'r', 'w', 'x' (rw)
    bool        watchable;
    bool        persist;        // host should save and restore on reconnect
    bool        has_min;
    bool        has_max;
    float       min_val;
    float       max_val;
    std::string options;        // space separated, seam/enum only
    std::string flags;          // space separated, seam/flags only
    std::string enabled_expr;   // CEL expression, empty = always enabled
    std::string visible_expr;   // CEL expression, empty = always visible
    std::string default_val;
};

} // namespace caps
} // namespace protocol
} // namespace seam