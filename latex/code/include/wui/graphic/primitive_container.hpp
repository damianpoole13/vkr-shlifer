
#pragma once

#include <wui/common/color.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/font.hpp>
#include <wui/common/error.hpp>
#include <wui/system/system_context.hpp>

#include <map>

#include <windows.h>


namespace wui
{

class primitive_container
{
public:
    primitive_container(wui::system_context &context_);
    ~primitive_container();

    void init();
    void release();

    wui::error get_error() const;

    HPEN get_pen(int32_t style, int32_t width, color color_);
    HBRUSH get_brush(color color_);
    HFONT get_font(font font_);
    HBITMAP get_bitmap(int32_t width, int32_t height, uint8_t *buffer, HDC hdc);

private:
    wui::system_context &context_;

    wui::error err;

    std::map<std::pair<std::pair<int32_t, int32_t>, color>, HPEN> pens;
    std::map<int32_t, HBRUSH> brushes;
    std::map<std::pair<std::pair<std::string, int32_t>, decorations>, HFONT> fonts;
    std::map<std::pair<int32_t, int32_t>, HBITMAP> bitmaps;
};

}
