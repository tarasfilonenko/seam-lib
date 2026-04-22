#pragma once
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