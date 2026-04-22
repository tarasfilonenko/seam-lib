#pragma once
// ─────────────────────────────────────────────
// seam::protocol::caps::Caps
//
// Models the CAPS block from a SEAM CAPS response.
// Top-level device capability description.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include "Group.h"

namespace seam {
namespace protocol {
namespace caps {

struct Caps {
    std::string        device_type;
    std::string        device_name;
    std::string        version;
    std::vector<Group> groups;
};

} // namespace caps
} // namespace protocol
} // namespace seam