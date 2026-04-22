#pragma once
#include <AUnit.h>

test(caps_minimal) {
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
    assertTrue(ev.has_value());
    assertEqual((int)EventType::CAPS_READY, (int)ev->type);
    auto* pl = std::get_if<CapsReadyPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("temperature_sensor",   pl->caps.device_type.c_str());
    assertEqual("Temperature Sensor",   pl->caps.device_name.c_str());
    assertEqual("1.0.0",                pl->caps.version.c_str());
    assertEqual((size_t)1,              pl->caps.groups.size());
    assertEqual((size_t)1,              pl->caps.groups[0].params.size());
    assertEqual("temp_c",               pl->caps.groups[0].params[0].id.c_str());
    assertEqual("seam/float",           pl->caps.groups[0].params[0].type.c_str());
}

test(caps_param_fields) {
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
    assertTrue(pl != nullptr);
    auto* param = &pl->caps.groups[0].params[0];
    assertTrue(param->watchable);
    assertTrue(param->persist);
    assertTrue(param->has_min);
    assertTrue(param->has_max);
    assertEqual("a b c",     param->options.c_str());
    assertEqual("f1 f2 f3",  param->flags.c_str());
    assertEqual("p > 0",     param->enabled_expr.c_str());
    assertEqual("p < 200",   param->visible_expr.c_str());
    assertEqual("150",       param->default_val.c_str());
    assertEqual('r',         param->access);
}

test(caps_action) {
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
    auto* pl = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
    assertTrue(pl != nullptr);
    auto* action = &pl->caps.groups[0].actions[0];
    assertEqual("sweep",     action->id.c_str());
    assertEqual("Sweep",     action->label.c_str());
    assertEqual((size_t)2,   action->args.size());
    assertEqual("start_us",  action->args[0].id.c_str());
    assertTrue(action->args[0].has_min);
}

test(caps_action_trigger) {
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
    auto* pl = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
    assertTrue(pl != nullptr);
    assertEqual("diagram", pl->caps.groups[0].actions[0].trigger.c_str());
}

test(caps_group_expressions) {
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
    auto* pl = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
    assertTrue(pl != nullptr);
    auto* group = &pl->caps.groups[0];
    assertEqual("mode == \"advanced\"", group->enabled_expr.c_str());
    assertEqual("show_advanced",        group->visible_expr.c_str());
}

test(caps_stream) {
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
    auto* pl = ev ? std::get_if<CapsReadyPayload>(&ev->payload) : nullptr;
    assertTrue(pl != nullptr);
    auto* stream = &pl->caps.groups[0].streams[0];
    assertEqual("position",   stream->id.c_str());
    assertEqual("seam/float", stream->type.c_str());
    assertEqual("streaming",  stream->enabled_expr.c_str());
}
