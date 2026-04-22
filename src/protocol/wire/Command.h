#pragma once
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include "In.h"

namespace seam {
namespace protocol {
namespace wire {

enum class CommandType {
    GET,
    SET,
    DO,
};

struct GetPayload {
    std::string id;
};

struct SetPayload {
    std::string          id;
    std::vector<uint8_t> data;
};

struct DoPayload {
    std::string     id;
    std::vector<In> args;
};

struct Command {
    CommandType type;
    std::variant <
        GetPayload,
        SetPayload,
        DoPayload
    > payload;
};

} // namespace wire
} // namespace protocol
} // namespace seam