
#pragma once

#include <cstdint>

namespace wui
{

enum class mouse_event_type
{
    move,
    enter,
    leave,
    right_down,
    right_up,
    center_down,
    center_up,
    left_down,
    left_up,
    left_double,
    wheel
};

struct mouse_event
{
    mouse_event_type type;

    int32_t x, y;

    int32_t wheel_delta;
};

}
