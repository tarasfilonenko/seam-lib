#!/usr/bin/env python3
# ─────────────────────────────────────────────
# test/capture.py
#
# Sends "start" to the board, captures AUnit serial output,
# exits 0 on pass, 1 on fail.
# ─────────────────────────────────────────────

import serial
import sys
import argparse
import time


def main():
    parser = argparse.ArgumentParser(
        description="Capture and evaluate AUnit test output from a serial device"
    )
    parser.add_argument("--port",    required=True,      help="Serial port")
    parser.add_argument("--baud",    type=int, default=115200)
    parser.add_argument("--timeout", type=int, default=30, help="Global timeout in seconds")
    args = parser.parse_args()

    passed = 0
    failed = 0
    done   = False

    print(f"==> Opening {args.port} at {args.baud} baud...")

    try:
        with serial.Serial(args.port, args.baud, timeout=1) as ser:
            time.sleep(2)
            ser.reset_input_buffer()
            ser.write(b"start\n")

            deadline = time.time() + args.timeout

            while time.time() < deadline:
                raw = ser.readline()
                if not raw:
                    continue

                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                print(line)

                if line.startswith("Test ") and line.endswith(" passed."):
                    passed += 1
                elif line.startswith("Test ") and line.endswith(" failed."):
                    failed += 1
                elif line.startswith("TestRunner summary:"):
                    done = True
                    break

    except serial.SerialException as e:
        print(f"ERROR: could not open port {args.port}: {e}")
        sys.exit(1)

    if not done:
        print(f"ERROR: timed out after {args.timeout}s waiting for TestRunner summary")
        sys.exit(1)

    print()
    print(f"{'PASSED' if failed == 0 else 'FAILED'}: {passed} passed, {failed} failed")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
