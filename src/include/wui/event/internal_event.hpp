

#pragma once

namespace wui
{

enum class internal_event_type
{
    window_created,

    set_focus,
    remove_focus,
    execute_focused,
    
    size_changed,
    position_changed,

    window_expanded,
    window_normalized,
    window_minimized,

    user_emitted
};

struct internal_event
{
    internal_event_type type;
    int32_t x, y;
};

}
