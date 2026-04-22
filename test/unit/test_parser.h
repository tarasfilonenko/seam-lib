#pragma once
#include <AUnit.h>

test(parser_changed) {
    Parser p;
    auto ev = feedAndTake(p, "CHANGED some_param\r\n");
    assertTrue(ev.has_value());
    assertEqual((int)EventType::CHANGED, (int)ev->type);
    auto* pl = std::get_if<ChangedPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("some_param", pl->id.c_str());
}

test(parser_ok_bare) {
    Parser p;
    auto ev = feedAndTake(p, "OK\r\n");
    assertTrue(ev.has_value());
    assertEqual((int)EventType::CMD_OK, (int)ev->type);
    auto* pl = std::get_if<OkPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("", pl->text.c_str());
}

test(parser_ok_text) {
    Parser p;
    auto ev = feedAndTake(p, "OK PWM enabled at 1500us\r\n");
    auto* pl = ev ? std::get_if<OkPayload>(&ev->payload) : nullptr;
    assertTrue(pl != nullptr);
    assertEqual("PWM enabled at 1500us", pl->text.c_str());
}

test(parser_err) {
    Parser p;
    auto ev = feedAndTake(p,
        "ERR BEGIN UNKNOWN_PARAM\r\n"
        "id:pulse_width_us\r\n"
        "message:not found\r\n"
        "ERR END\r\n");
    assertTrue(ev.has_value());
    assertEqual((int)EventType::CMD_ERR, (int)ev->type);
    auto* pl = std::get_if<ErrPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("UNKNOWN_PARAM",   pl->code.c_str());
    assertEqual("pulse_width_us",  pl->id.c_str());
    assertEqual("not found",       pl->message.c_str());
}

test(parser_err_out_of_range) {
    Parser p;
    auto ev = feedAndTake(p,
        "ERR BEGIN OUT_OF_RANGE\r\n"
        "id:pulse_width_us\r\n"
        "min:500\r\n"
        "max:2500\r\n"
        "ERR END\r\n");
    auto* pl = ev ? std::get_if<ErrPayload>(&ev->payload) : nullptr;
    assertTrue(pl != nullptr);
    assertEqual("OUT_OF_RANGE", pl->code.c_str());
    assertFalse(pl->min.empty());
    assertFalse(pl->max.empty());
}

test(parser_parse_error) {
    Parser p;
    auto ev = feedAndTake(p, "GARBAGE\r\n");
    assertTrue(ev.has_value());
    assertEqual((int)EventType::PARSE_ERROR, (int)ev->type);
    auto* pl = std::get_if<ParseErrorPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertFalse(pl->line.empty());
    assertFalse(pl->reason.empty());
}

test(parser_changed_missing_id) {
    Parser p;
    auto ev = feedAndTake(p, "CHANGED\r\n");
    assertTrue(ev.has_value());
    assertEqual((int)EventType::PARSE_ERROR, (int)ev->type);
}

test(parser_value_missing_args) {
    Parser p;
    auto ev = feedAndTake(p, "VALUE\r\n");
    assertTrue(ev.has_value());
    assertEqual((int)EventType::PARSE_ERROR, (int)ev->type);
}
