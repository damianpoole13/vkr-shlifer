

#pragma once

#include <wui/system/system_context.hpp>

#include <wui/common/color.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/font.hpp>
#include <wui/common/error.hpp>

#include <wui/graphic/primitive_container.hpp>

#include <string_view>
#include <cstdint>

namespace wui
{

class graphic
{
public:
    graphic(system_context &context);
    ~graphic();

    bool init(const rect &max_size, color background_color);
    void release();

    void set_background_color(color background_color);

    void clear(const rect &position);

    void flush(const rect &updated_size);

    void draw_pixel(const rect &position, color color_);

    void draw_line(const rect &position, color color_, uint32_t width = 1);

    rect measure_text(std::string_view text, const font &font_);
    void draw_text(const rect &position, std::string_view text, color color_, const font &font_);

    void draw_rect(const rect &position, color fill_color);
    void draw_rect(const rect &position, color border_color, color fill_color, uint32_t border_width, uint32_t round);

    /// draw some buffer on context
    void draw_buffer(const rect &position, uint8_t *buffer, int32_t left_shift, int32_t top_shift);

    /// draw another graphic on context
    void draw_graphic(const rect &position, graphic &graphic_, int32_t left_shift, int32_t top_shift);

    HDC drawable();

    error get_error() const;

private:
    system_context &context_;

    primitive_container pc;

    rect max_size;

    color background_color;

    HDC mem_dc;
    HBITMAP mem_bitmap;

    error err;
};

}
