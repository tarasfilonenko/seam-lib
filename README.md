# seam-lib

Reusable C++ primitives for SEAM ecosystem firmware.

## Contents

- `src/rtos/OwningQueue.h` — FreeRTOS queue for heap-allocated objects
- `src/rtos/ValueQueue.h` — FreeRTOS queue for plain structs
- `src/rtos/RingBuffer.h` — FreeRTOS ring buffer for raw bytes

## Requirements

- FreeRTOS
- C++23
- ESP32 Arduino core v3.x or equivalent

## Adding to a project

```bash
git submodule add https://github.com/tarasfilonenko/seam-lib lib/seam-lib
git add .gitmodules lib/seam-lib
git commit -m "chore: add seam-lib submodule"
```

Add the include path to your Makefile:

```makefile
EXTRA_FLAGS = -I$(shell pwd)/lib/seam-lib/src
```

## Cloning a project that uses seam-lib

```bash
git clone --recurse-submodules <repo-url>

# or if already cloned
git submodule update --init --recursive
```

## Usage

```cpp
#include "rtos/OwningQueue.h"
#include "rtos/ValueQueue.h"
#include "rtos/RingBuffer.h"

seam::rtos::OwningQueue<MyEvent> eventQueue(32);
seam::rtos::ValueQueue<MyStruct> valueQueue(16);
seam::rtos::RingBuffer           ringBuffer(4096);
```