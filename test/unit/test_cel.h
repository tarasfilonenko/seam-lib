#pragma once
// ─────────────────────────────────────────────
// test_cel — umbrella for the seam::cel test suite
//
// Per-feature test files live in test/unit/cel/ and are pulled in
// here. The unit sketch (unit.ino) only needs to #include this file
// to register every cel test() with AUnit.
//
// New cel features get their own test/unit/cel/<feature>.h and are
// added to the include list below.
// ─────────────────────────────────────────────

#include "cel/helpers.h"

#include "cel/empty.h"
#include "cel/bool.h"
#include "cel/number.h"
#include "cel/string.h"
#include "cel/identifier.h"
#include "cel/paren.h"
#include "cel/not.h"
#include "cel/cmp.h"
