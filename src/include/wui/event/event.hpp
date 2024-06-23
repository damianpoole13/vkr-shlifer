

#pragma once

#include <wui/event/mouse_event.hpp>
#include <wui/event/keyboard_event.hpp>
#include <wui/event/internal_event.hpp>
#include <wui/event/system_event.hpp>

namespace wui
{

enum class event_type : uint32_t
{
    system = (1 << 0),
    mouse = (1 << 1),
    keyboard = (1 << 2),
    internal = (1 << 3),

    all = system | mouse | keyboard | internal
};

struct event
{
    event_type type;

    union
    {
        mouse_event mouse_event_;
        keyboard_event keyboard_event_;
        internal_event internal_event_;
        system_event system_event_;
    };
};

}
