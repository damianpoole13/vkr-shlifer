
#include <wui/graphic/graphic.hpp>
#include <wui/common/flag_helpers.hpp>
#include <wui/system/tools.hpp>

#include <boost/nowide/convert.hpp>

namespace wui
{

graphic::graphic(system_context &context__)
    : context_(context__),
      pc(context_),
      max_size(),
      background_color(0)
    , mem_dc(0),
      mem_bitmap(0),

    err{}
{
}

graphic::~graphic()
{
    release();
}

bool graphic::init(const rect &max_size_, color background_color_)
{
    max_size = max_size_;
    background_color = background_color_;

    if (mem_dc)
    {
        err.type = error_type::already_runned;
        err.component = "graphic::init()";
        return false;
    }

    err.reset();

    auto wnd_dc = GetDC(context_.hwnd);

    mem_dc = CreateCompatibleDC(wnd_dc);

    mem_bitmap = CreateCompatibleBitmap(wnd_dc, max_size.width(), max_size.height());
    if (!mem_bitmap)
    {
        err.type = error_type::no_handle;
        err.component = "graphic::init()";
        err.message = "CreateCompatibleBitmap returns null";

        ReleaseDC(context_.hwnd, wnd_dc);

        return false;
    }

    SelectObject(mem_dc, mem_bitmap);

    SetMapMode(mem_dc, MM_TEXT);

    RECT filling_rect = { 0, 0, max_size.width(), max_size.height() };
    FillRect(mem_dc, &filling_rect, pc.get_brush(background_color));

    ReleaseDC(context_.hwnd, wnd_dc);

    pc.init();

    return true;
}

void graphic::release()
{
    DeleteObject(mem_bitmap);
    mem_bitmap = 0;

    DeleteDC(mem_dc);
    mem_dc = 0;

    pc.release();
}

void graphic::set_background_color(color background_color_)
{
    background_color = background_color_;

    clear({ 0, 0, max_size.width(), max_size.height() });
}

void graphic::clear(const rect &position)
{
    if (!mem_dc)
    {
        return;
    }

    RECT filling_rect = { position.left, position.top, position.right, position.bottom };
    FillRect(mem_dc, &filling_rect, pc.get_brush(background_color));
}

void graphic::flush(const rect &updated_size)
{
    auto wnd_dc = GetDC(context_.hwnd);

    if (wnd_dc)
    {
        BitBlt(wnd_dc,
            updated_size.left,
            updated_size.top,
            updated_size.width(),
            updated_size.height(),
            mem_dc,
            updated_size.left,
            updated_size.top,
            SRCCOPY);
    }

    ReleaseDC(context_.hwnd, wnd_dc);
}

void graphic::draw_pixel(const rect &position, color color_)
{
    SetPixel(mem_dc, position.left, position.top, color_);
}

void graphic::draw_line(const rect &position, color color_, uint32_t width)
{
    auto old_pen = (HPEN)SelectObject(mem_dc, pc.get_pen(PS_SOLID, width, color_));

    MoveToEx(mem_dc, position.left, position.top, (LPPOINT)NULL);
    LineTo(mem_dc, position.right, position.bottom);

    SelectObject(mem_dc, old_pen);
}

rect graphic::measure_text(std::string_view text_, const font &font__)
{
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font__));

    RECT text_rect = { 0 };
    auto wide_str = boost::nowide::widen(text_);
    DrawTextW(mem_dc, wide_str.c_str(), static_cast<int32_t>(wide_str.size()), &text_rect, DT_CALCRECT);

    SelectObject(mem_dc, old_font);

    return {0, 0, text_rect.right, text_rect.bottom};
}

void graphic::draw_text(const rect &position, std::string_view text_, color color_, const font &font__)
{
    auto old_font = (HFONT)SelectObject(mem_dc, pc.get_font(font__));
    
    SetTextColor(mem_dc, color_);
    SetBkMode(mem_dc, TRANSPARENT);

    auto wide_str = boost::nowide::widen(text_);
    TextOutW(mem_dc, position.left, position.top, wide_str.c_str(), static_cast<int32_t>(wide_str.size()));

    SelectObject(mem_dc, old_font);
}

void graphic::draw_rect(const rect &position, color fill_color)
{
    RECT position_rect = { position.left, position.top, position.right, position.bottom };
    FillRect(mem_dc, &position_rect, pc.get_brush(fill_color));
}

void graphic::draw_rect(const rect &position, color border_color, color fill_color, uint32_t border_width, uint32_t rnd)
{
    auto old_pen = (HPEN)SelectObject(mem_dc, pc.get_pen(border_width != 0 ? PS_SOLID : PS_NULL, border_width, border_color));

    auto old_brush = (HBRUSH)SelectObject(mem_dc, pc.get_brush(fill_color));

    RoundRect(mem_dc, position.left, position.top, position.right, position.bottom, rnd, rnd);

    SelectObject(mem_dc, old_brush);

    SelectObject(mem_dc, old_pen);
}

void graphic::draw_buffer(const rect &position, uint8_t *buffer, int32_t left_shift, int32_t top_shift)
{
    auto source_bitmap = pc.get_bitmap(position.width(), position.height(), buffer, mem_dc);
    auto source_dc = CreateCompatibleDC(mem_dc);
    SelectObject(source_dc, source_bitmap);

    BitBlt(mem_dc,
        position.left,
        position.top,
        position.width(),
        position.height(),
        source_dc,
        left_shift,
        top_shift,
        SRCCOPY);

    DeleteDC(source_dc);
}

void graphic::draw_graphic(const rect &position, graphic &graphic_, int32_t left_shift, int32_t top_shift)
{
    if (graphic_.drawable())
    {
        BitBlt(mem_dc,
            position.left,
            position.top,
            position.right,
            position.bottom,
            graphic_.drawable(),
            left_shift,
            top_shift,
            SRCCOPY);
    }
}

HDC graphic::drawable()
{
    return mem_dc;
}

error graphic::get_error() const
{
    return err;
}

}
