#pragma once
// ─────────────────────────────────────────────
// seam::protocol::wire::In
//
// Models an IN frame within a DO command.
// Carries a named argument value as raw bytes.
//
// Wire format:
//   IN <id> <length>\r\n
//   <data>\r\n
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include <cstdint>

namespace seam {
namespace protocol {
namespace wire {

struct In {
    std::string          id;
    std::vector<uint8_t> data;
};

} // namespace wire
} // namespace protocol
} // namespace seam