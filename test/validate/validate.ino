// Minimal compile validation for seam-lib/src/rtos
// Instantiates all three classes to force template compilation.
// Not intended to run — compile only.

#include "rtos/OwningQueue.h"
#include "rtos/ValueQueue.h"
#include "rtos/RingBuffer.h"

#include "protocol/protocol.h"

using namespace seam::rtos;

// dummy types
struct MyEvent {
    int id;
};

struct MyValue {
    float x;
    float y;
};

OwningQueue<MyEvent> owningQueue(32);
ValueQueue<MyValue>  valueQueue(16);
RingBuffer           ringBuffer(4096);

void setup() {
    auto writer = owningQueue.takeWriter();
    auto reader = owningQueue.takeReader();

    auto vwriter = valueQueue.takeWriter();
    auto vreader = valueQueue.takeReader();

    auto rwriter = ringBuffer.takeWriter();
    auto rreader = ringBuffer.takeReader();
}

void loop() {}