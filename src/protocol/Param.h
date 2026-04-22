#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Param
//
// Models a PARAM block from a SEAM CAPS response.
// ─────────────────────────────────────────────

#include <string>
#include <vector>

namespace seam {
namespace protocol {

struct Param {
    std::string id;
    std::string label;
    std::string description;
    std::string type;           // MIME type e.g. seam/int, image/png
    char        access;         // 'r', 'w', 'x' (rw)
    bool        watchable;
    bool        has_min;
    bool        has_max;
    float       min_val;
    float       max_val;
    std::string options;        // space separated, seam/enum only
    std::string enabled_expr;   // CEL expression, empty = always enabled
    std::string default_val;
};

} // namespace protocol
} // namespace seam