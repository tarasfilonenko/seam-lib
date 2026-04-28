// Minimal compile validation for seam-lib/src/
// Instantiates all classes to force template compilation.
// Not intended to run — compile only.

#include "rtos/OwningQueue.h"
#include "rtos/ValueQueue.h"
#include "rtos/RingBuffer.h"
#include "protocol/protocol.h"
#include "protocol/Parser.h"
#include "protocol/Serializer.h"

using namespace seam::rtos;

// ── rtos types ───────────────────────────────

struct MyEvent { int id; };
struct MyValue { float x; float y; };

OwningQueue<MyEvent> owningQueue(32);
ValueQueue<MyValue>  valueQueue(16);
RingBuffer           ringBuffer(4096);

// ── caps types ───────────────────────────────

seam::protocol::caps::Param   param;
seam::protocol::caps::Arg     arg;
seam::protocol::caps::Action  action;
seam::protocol::caps::Stream  stream;
seam::protocol::caps::Group   group;
seam::protocol::caps::Caps    caps;

// ── wire types ───────────────────────────────

seam::protocol::wire::In      in;
seam::protocol::wire::Command command;
seam::protocol::wire::Event   event;

// ── parser ───────────────────────────────────

seam::protocol::Parser parser;
seam::protocol::Serializer serializer;

void setup() {
    // rtos
    auto owningWriter = owningQueue.takeWriter();
    auto owningReader = owningQueue.takeReader();
    auto valueWriter  = valueQueue.takeWriter();
    auto valueReader  = valueQueue.takeReader();
    auto ringWriter   = ringBuffer.takeWriter();
    auto ringReader   = ringBuffer.takeReader();

    // parser — feed some bytes and drain events
    const char* sample = "CHANGED some_param\r\n";
    parser.feed(reinterpret_cast<const uint8_t*>(sample), strlen(sample));
    while (auto ev = parser.takeEvent()) {
        (void)ev;
    }

    parser.reset();

    command.type = seam::protocol::wire::CommandType::GET;
    command.payload = seam::protocol::wire::GetPayload{ "some_param" };
    auto encoded = serializer.serialize(command);
    if (encoded) {
        (void)encoded->size();
    }
}

void loop() {}
