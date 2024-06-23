

#pragma once

namespace wui
{

enum class system_event_type
{
    device_change
};

struct system_event
{
    system_event_type type;
    int32_t x, y;
};

}
