#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Stream
//
// Models a STREAM block from a SEAM CAPS response.
// ─────────────────────────────────────────────

#include <string>

namespace seam {
namespace protocol {

struct Stream {
    std::string id;
    std::string label;
    std::string description;
    std::string type;           // MIME type
    std::string enabled_expr;   // CEL expression, empty = always enabled
};

} // namespace protocol
} // namespace seam