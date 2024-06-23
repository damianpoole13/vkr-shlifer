

#pragma once

#include <string>
#include <wui/system/system_context.hpp>

namespace wui
{

/// Clipboard's functions
void clipboard_put(std::string_view text, system_context &context);

bool is_text_in_clipboard(system_context &context);
std::string clipboard_get_text(system_context &context);

}
