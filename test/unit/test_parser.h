#pragma once
// ─────────────────────────────────────────────
// test_parser.h — basic parser tests
// ─────────────────────────────────────────────

void runParserTests() {
    Serial.println("-- parser --");

    // CHANGED
    {
        Parser p;
        auto ev = feedAndTake(p, "CHANGED some_param\r\n");
        check("parser_changed_event",
            ev && ev->type == EventType::CHANGED,
            "expected CHANGED event");
        auto* pl = ev ? std::get_if<ChangedPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("parser_changed_id",
            "some_param",
            pl ? pl->id : "");
    }

    // OK bare
    {
        Parser p;
        auto ev = feedAndTake(p, "OK\r\n");
        check("parser_ok_type",
            ev && ev->type == EventType::CMD_OK,
            "expected CMD_OK");
        auto* pl = ev ? std::get_if<OkPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("parser_ok_text_empty",
            "", pl ? pl->text : "MISSING");
    }

    // OK with trailing text
    {
        Parser p;
        auto ev = feedAndTake(p, "OK PWM enabled at 1500us\r\n");
        auto* pl = ev ? std::get_if<OkPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("parser_ok_text",
            "PWM enabled at 1500us",
            pl ? pl->text : "");
    }

    // ERR block
    {
        Parser p;
        auto ev = feedAndTake(p,
            "ERR BEGIN UNKNOWN_PARAM\r\n"
            "id:pulse_width_us\r\n"
            "message:not found\r\n"
            "ERR END\r\n");
        check("parser_err_type",
            ev && ev->type == EventType::CMD_ERR,
            "expected CMD_ERR");
        auto* pl = ev ? std::get_if<ErrPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("parser_err_code", "UNKNOWN_PARAM", pl ? pl->code : "");
        checkEqual<std::string>("parser_err_id",   "pulse_width_us", pl ? pl->id : "");
        checkEqual<std::string>("parser_err_msg",  "not found", pl ? pl->message : "");
    }

    // ERR OUT_OF_RANGE with min/max
    {
        Parser p;
        auto ev = feedAndTake(p,
            "ERR BEGIN OUT_OF_RANGE\r\n"
            "id:pulse_width_us\r\n"
            "min:500\r\n"
            "max:2500\r\n"
            "ERR END\r\n");
        auto* pl = ev ? std::get_if<ErrPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("parser_err_out_of_range_code", "OUT_OF_RANGE", pl ? pl->code : "");
        check("parser_err_out_of_range_min",
            pl && !pl->min.empty(),
            "expected min field");
        check("parser_err_out_of_range_max",
            pl && !pl->max.empty(),
            "expected max field");
    }

    // PARSE_ERROR on unknown keyword
    {
        Parser p;
        auto ev = feedAndTake(p, "GARBAGE\r\n");
        check("parser_parse_error",
            ev && ev->type == EventType::PARSE_ERROR,
            "expected PARSE_ERROR");
        auto* pl = ev ? std::get_if<ParseErrorPayload>(&ev->payload) : nullptr;
        check("parser_parse_error_has_line",
            pl && !pl->line.empty(),
            "expected line field");
        check("parser_parse_error_has_reason",
            pl && !pl->reason.empty(),
            "expected reason field");
    }

    // PARSE_ERROR on malformed CHANGED
    {
        Parser p;
        auto ev = feedAndTake(p, "CHANGED\r\n");
        check("parser_changed_missing_id",
            ev && ev->type == EventType::PARSE_ERROR,
            "expected PARSE_ERROR for CHANGED with no id");
    }

    // PARSE_ERROR on malformed VALUE
    {
        Parser p;
        auto ev = feedAndTake(p, "VALUE\r\n");
        check("parser_value_missing_args",
            ev && ev->type == EventType::PARSE_ERROR,
            "expected PARSE_ERROR for VALUE with no args");
    }
}