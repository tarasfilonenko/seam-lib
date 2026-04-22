#pragma once
// ─────────────────────────────────────────────
// test_caps.h — CAPS parsing tests
// ─────────────────────────────────────────────

void runCapsTests() {
    Serial.println("-- caps --");

    // minimal CAPS
    {
        Parser p;
        auto ev = feedAndTake(p,
            "CAPS BEGIN\r\n"
            "type:temperature_sensor\r\n"
            "name:Temperature Sensor\r\n"
            "version:1.0.0\r\n"
            "GROUP BEGIN readings\r\n"
            "label:Readings\r\n"
            "PARAM BEGIN temp_c\r\n"
            "type:seam/float\r\n"
            "access:r\r\n"
            "label:Temperature\r\n"
            "PARAM END\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n");

        check("caps_type",
            ev && ev->type == EventType::CAPS_READY,
            "expected CAPS_READY");
        auto* pl = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("caps_device_type", "temperature_sensor",
            pl ? pl->caps.device_type : "");
        checkEqual<std::string>("caps_device_name", "Temperature Sensor",
            pl ? pl->caps.device_name : "");
        checkEqual<std::string>("caps_version", "1.0.0",
            pl ? pl->caps.version : "");
        checkEqual<size_t>("caps_group_count", 1,
            pl ? pl->caps.groups.size() : 0);
        checkEqual<size_t>("caps_param_count", 1,
            pl ? pl->caps.groups[0].params.size() : 0);
        checkEqual<std::string>("caps_param_id", "temp_c",
            pl ? pl->caps.groups[0].params[0].id : "");
        checkEqual<std::string>("caps_param_type", "seam/float",
            pl ? pl->caps.groups[0].params[0].type : "");
    }

    // PARAM fields — watchable, persist, min, max, options, flags
    {
        Parser p;
        auto ev = feedAndTake(p,
            "CAPS BEGIN\r\n"
            "type:test\r\nname:Test\r\nversion:1.0.0\r\n"
            "GROUP BEGIN g\r\nlabel:G\r\n"
            "PARAM BEGIN p\r\n"
            "type:seam/int\r\n"
            "access:rw\r\n"
            "label:P\r\n"
            "watchable:true\r\n"
            "persist:true\r\n"
            "min:100\r\n"
            "max:200\r\n"
            "options:a b c\r\n"
            "flags:f1 f2 f3\r\n"
            "enabled:p > 0\r\n"
            "visible:p < 200\r\n"
            "default:150\r\n"
            "PARAM END\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n");

        auto* pl = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
        auto* param = pl ? &pl->caps.groups[0].params[0] : nullptr;

        checkEqual<bool>("caps_param_watchable", true,  param ? param->watchable : false);
        checkEqual<bool>("caps_param_persist",   true,  param ? param->persist   : false);
        checkEqual<bool>("caps_param_has_min",   true,  param ? param->has_min   : false);
        checkEqual<bool>("caps_param_has_max",   true,  param ? param->has_max   : false);
        checkEqual<std::string>("caps_param_options", "a b c", param ? param->options : "");
        checkEqual<std::string>("caps_param_flags",   "f1 f2 f3", param ? param->flags : "");
        checkEqual<std::string>("caps_param_enabled", "p > 0",   param ? param->enabled_expr : "");
        checkEqual<std::string>("caps_param_visible", "p < 200", param ? param->visible_expr : "");
        checkEqual<std::string>("caps_param_default", "150",     param ? param->default_val : "");
        checkEqual<char>("caps_param_access", 'r', param ? param->access : ' ');
    }

    // ACTION with ARGs
    {
        Parser p;
        auto ev = feedAndTake(p,
            "CAPS BEGIN\r\n"
            "type:test\r\nname:Test\r\nversion:1.0.0\r\n"
            "GROUP BEGIN g\r\nlabel:G\r\n"
            "ACTION BEGIN sweep\r\n"
            "label:Sweep\r\n"
            "description:Sweep action\r\n"
            "enabled:mode == \"sweep\"\r\n"
            "visible:true\r\n"
            "ARG BEGIN start_us\r\n"
            "type:seam/int\r\n"
            "label:Start\r\n"
            "min:500\r\n"
            "max:2500\r\n"
            "ARG END\r\n"
            "ARG BEGIN end_us\r\n"
            "type:seam/int\r\n"
            "label:End\r\n"
            "ARG END\r\n"
            "ACTION END\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n");

        auto* pl     = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
        auto* action = pl ? &pl->caps.groups[0].actions[0] : nullptr;

        checkEqual<std::string>("caps_action_id",      "sweep",  action ? action->id : "");
        checkEqual<std::string>("caps_action_label",   "Sweep",  action ? action->label : "");
        checkEqual<size_t>     ("caps_action_arg_count", 2,      action ? action->args.size() : 0);
        checkEqual<std::string>("caps_action_arg0_id", "start_us",
            action ? action->args[0].id : "");
        checkEqual<bool>       ("caps_action_arg0_has_min", true,
            action ? action->args[0].has_min : false);
    }

    // ACTION trigger field
    {
        Parser p;
        auto ev = feedAndTake(p,
            "CAPS BEGIN\r\n"
            "type:test\r\nname:Test\r\nversion:1.0.0\r\n"
            "GROUP BEGIN g\r\nlabel:G\r\n"
            "ACTION BEGIN on_click\r\n"
            "label:Click\r\n"
            "trigger:diagram\r\n"
            "ACTION END\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n");

        auto* pl     = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
        auto* action = pl ? &pl->caps.groups[0].actions[0] : nullptr;
        checkEqual<std::string>("caps_action_trigger", "diagram",
            action ? action->trigger : "");
    }

    // GROUP visible and enabled expressions
    {
        Parser p;
        auto ev = feedAndTake(p,
            "CAPS BEGIN\r\n"
            "type:test\r\nname:Test\r\nversion:1.0.0\r\n"
            "GROUP BEGIN g\r\n"
            "label:G\r\n"
            "enabled:mode == \"advanced\"\r\n"
            "visible:show_advanced\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n");

        auto* pl    = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
        auto* group = pl ? &pl->caps.groups[0] : nullptr;
        checkEqual<std::string>("caps_group_enabled", "mode == \"advanced\"",
            group ? group->enabled_expr : "");
        checkEqual<std::string>("caps_group_visible", "show_advanced",
            group ? group->visible_expr : "");
    }

    // STREAM block
    {
        Parser p;
        auto ev = feedAndTake(p,
            "CAPS BEGIN\r\n"
            "type:test\r\nname:Test\r\nversion:1.0.0\r\n"
            "GROUP BEGIN g\r\nlabel:G\r\n"
            "STREAM BEGIN position\r\n"
            "type:seam/float\r\n"
            "label:Position\r\n"
            "description:Live position\r\n"
            "enabled:streaming\r\n"
            "STREAM END\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n");

        auto* pl     = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
        auto* stream = pl ? &pl->caps.groups[0].streams[0] : nullptr;
        checkEqual<std::string>("caps_stream_id",      "position",    stream ? stream->id : "");
        checkEqual<std::string>("caps_stream_type",    "seam/float",  stream ? stream->type : "");
        checkEqual<std::string>("caps_stream_enabled", "streaming",   stream ? stream->enabled_expr : "");
    }
}