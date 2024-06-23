
#include <wui/graphic/primitive_container.hpp>

#include <wui/common/flag_helpers.hpp>

#include <boost/nowide/convert.hpp>

namespace wui
{

primitive_container::primitive_container(wui::system_context &context__)
    : context_(context__)
{
}

primitive_container::~primitive_container()
{
    release();
}


void primitive_container::init()
{
}

void primitive_container::release()
{
    for (auto &p : pens)
    {
        DeleteObject(p.second);
    }
    pens.clear();

    for (auto &b : brushes)
    {
        DeleteObject(b.second);
    }
    brushes.clear();

    for (auto &f : fonts)
    {
        DeleteObject(f.second);
    }
    fonts.clear();

    for (auto &b : bitmaps)
    {
        DeleteObject(b.second);
    }
    bitmaps.clear();
}

HPEN primitive_container::get_pen(int32_t style, int32_t width, color color_)
{
    auto it = pens.find({ { style, width }, color_ });
    if (it != pens.end())
    {
        return it->second;
    }
    auto pen = CreatePen(style, width, color_);

    pens[{ { style, width }, color_ }] = pen;

    return pen;
}

HBRUSH primitive_container::get_brush(color color_)
{
    if (get_alpha(color_) != 0)
    {
        return (HBRUSH)GetStockObject(NULL_BRUSH);
    }
    auto it = brushes.find(color_);
    if (it != brushes.end())
    {
        return it->second;
    }

    auto brush = CreateSolidBrush(color_);

    brushes[color_] = brush;

    return brush;
}

HFONT primitive_container::get_font(font font_)
{
    auto it = fonts.find({ {font_.name, font_.size }, font_.decorations_ });
    if (it != fonts.end())
    {
        return it->second;
    }

    LOGFONTW log_font = { font_.size,
        0,
        0,
        0,
        flag_is_set(font_.decorations_, decorations::bold) ? FW_MEDIUM : FW_DONTCARE,
        flag_is_set(font_.decorations_, decorations::italic),
        flag_is_set(font_.decorations_, decorations::underline),
        flag_is_set(font_.decorations_, decorations::strike_out),
        ANSI_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        0
    };
    auto font_name = boost::nowide::widen(font_.name);
    memcpy(log_font.lfFaceName, font_name.c_str(), font_name.size() * 2);
    HFONT font__ = CreateFontIndirectW(&log_font);

    fonts[{ {font_.name, font_.size }, font_.decorations_ }] = font__;

    return font__;
}

HBITMAP primitive_container::get_bitmap(int32_t width, int32_t height, uint8_t *buffer, HDC hdc)
{
    auto it = bitmaps.find({ width, height });
    if (it != bitmaps.end())
    {
        auto bitmap = it->second;

        BITMAPINFO bmpInfo;

        bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFO) - sizeof(RGBQUAD);
        bmpInfo.bmiHeader.biWidth = width;
        bmpInfo.bmiHeader.biHeight = 0 - (int)height;
        bmpInfo.bmiHeader.biPlanes = 1;
        bmpInfo.bmiHeader.biBitCount = 32;
        bmpInfo.bmiHeader.biCompression = BI_RGB;
        bmpInfo.bmiHeader.biSizeImage = 0;
        bmpInfo.bmiHeader.biXPelsPerMeter = 0;
        bmpInfo.bmiHeader.biYPelsPerMeter = 0;
        bmpInfo.bmiHeader.biClrUsed = 0;
        bmpInfo.bmiHeader.biClrImportant = 0;

        SetDIBits(hdc, bitmap, 0, height, buffer, &bmpInfo, DIB_RGB_COLORS);

        return bitmap;
    }

    auto bitmap = CreateBitmap(width, height, 1, 32, buffer);

    bitmaps[{ width, height }] = bitmap;

    return bitmap;
}

wui::error primitive_container::get_error() const
{
    return err;
}

}
