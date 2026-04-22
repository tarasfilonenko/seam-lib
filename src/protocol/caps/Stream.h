#pragma once
// ─────────────────────────────────────────────
// seam::protocol::caps::Stream
//
// Models a STREAM block from a SEAM CAPS response.
// Describes a named data channel emitted by the device.
// No values — see seam::protocol::wire::Event for data exchange.
// ─────────────────────────────────────────────

#include <string>

namespace seam {
namespace protocol {
namespace caps {

struct Stream {
    std::string id;
    std::string label;
    std::string description;
    std::string type;           // MIME type
    std::string enabled_expr;   // CEL expression, empty = always enabled
};

} // namespace caps
} // namespace protocol
} // namespace seam