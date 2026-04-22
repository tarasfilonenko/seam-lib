#!/usr/bin/env python3
# ─────────────────────────────────────────────
# test/capture.py
#
# Captures serial output from a connected board,
# parses TEST/DONE lines, exits 0 on pass, 1 on fail.
# ─────────────────────────────────────────────

import serial
import sys
import argparse
import time


def main():
    parser = argparse.ArgumentParser(
        description="Capture and evaluate unit test output from a serial device"
    )
    parser.add_argument("--port",    required=True,       help="Serial port")
    parser.add_argument("--baud",    type=int, default=115200)
    parser.add_argument("--timeout", type=int, default=10, help="Global timeout in seconds")
    args = parser.parse_args()

    passed        = 0
    failed        = 0
    done          = False
    first_output  = None
    first_test    = None

    print(f"==> Opening {args.port} at {args.baud} baud...")

    try:
        with serial.Serial(args.port, args.baud, timeout=1) as ser:
            # wait for board reset after upload
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

                # track first output time
                if first_output is None:
                    first_output = time.time()

                print(line)

                if line.startswith("TEST "):
                    # track first test line time
                    if first_test is None:
                        first_test = time.time()

                    if line.endswith("PASS"):
                        passed += 1
                    elif "FAIL" in line:
                        failed += 1

                elif line.startswith("DONE"):
                    done = True
                    break

                # early timeout — output received but no TEST line within 3s
                if (first_output is not None and
                    first_test is None and
                    time.time() - first_output > 3.0):
                    print("ERROR: board connected but no TEST output received within 3s")
                    sys.exit(1)

    except serial.SerialException as e:
        print(f"ERROR: could not open port {args.port}: {e}")
        sys.exit(1)

    if not done:
        print(f"ERROR: timed out after {args.timeout}s waiting for DONE")
        sys.exit(1)

    print()
    print(f"{'PASSED' if failed == 0 else 'FAILED'}: {passed} passed, {failed} failed")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()