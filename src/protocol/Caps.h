#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Caps
//
// Models the CAPS block from a SEAM CAPS response.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include "Group.h"

namespace seam {
namespace protocol {

struct Caps {
    std::string        device_type;
    std::string        device_name;
    std::string        version;
    std::vector<Group> groups;
};

} // namespace protocol
} // namespace seam