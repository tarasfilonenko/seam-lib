#pragma once
// ─────────────────────────────────────────────
// seam::protocol::In
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

namespace seam {
namespace protocol {

struct In {
    std::string          id;
    std::vector<uint8_t> data;
};

} // namespace protocol
} // namespace seam