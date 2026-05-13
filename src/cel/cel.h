#pragma once
// ─────────────────────────────────────────────
// seam::cel — single include
//
// Tiny CEL subset evaluator used by the SEAM CAPS protocol to gate
// visibility/enabledness of groups, params, actions and streams via
// their `visible_expr` / `enabled_expr` strings.
//
// Include this header to get the full public surface:
//   - seam::cel::Value      (Value.h)
//   - seam::cel::Env        (Env.h)
//   - seam::cel::Expression (Expression.h)
//   - seam::cel::compile    (Compile.h)
//   - seam::cel::evaluate   (Evaluate.h)
//
// Typical host wiring (e.g. dock ModuleModel):
//
//     // Once, on caps load:
//     seam::cel::Expression expr;
//     seam::cel::CompileError err;
//     if (!seam::cel::compile(param.visible_expr, expr, err)) {
//         log_warn("cel parse: %s @ %zu — %s",
//                  param.visible_expr.c_str(), err.position, err.message.c_str());
//     }
//     for (const auto &id : expr.references()) {
//         _reverse_index[id].push_back(&expr);
//     }
//
//     // On every value-change wave:
//     seam::cel::Env env{ this, &Self::lookupIdentifier };
//     seam::cel::Value v = seam::cel::evaluate(expr, env);
//     bool visible = v.isUndefined() ? false
//                  : v.isBool()      ? v.asBool()
//                                    : true;   // fail-open
// ─────────────────────────────────────────────

#include "Value.h"
#include "Env.h"
#include "Expression.h"
#include "Compile.h"
#include "Evaluate.h"
#include "Format.h"
