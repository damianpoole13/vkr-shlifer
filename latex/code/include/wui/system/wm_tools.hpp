
#pragma once

#include <wui/system/system_context.hpp>

#include <wui/common/rect.hpp>

namespace wui
{

/// Hide the WM's task bar icon
void hide_taskbar_icon(system_context &ctx);

/// Show the WM's task bar icon
void show_taskbar_icon(system_context &ctx);

/// Determine screen dimensions
rect get_screen_size(system_context &context);

}
