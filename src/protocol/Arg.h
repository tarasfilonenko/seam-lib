#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Arg
//
// Models an ARG block from a SEAM CAPS response.
// Declares a named argument for an ACTION.
// ─────────────────────────────────────────────

#include <string>

namespace seam {
namespace protocol {

struct Arg {
    std::string id;
    std::string label;
    std::string description;
    std::string type;           // MIME type
    bool        has_min;
    bool        has_max;
    float       min_val;
    float       max_val;
    std::string options;        // space separated, seam/enum only
};

} // namespace protocol
} // namespace seam