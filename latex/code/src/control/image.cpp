
#include <wui/control/image.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>
#include <wui/system/path_tools.hpp>

#include <boost/nowide/convert.hpp>

#include <cstring>

void load_image_from_data(const std::vector<uint8_t> &data, Gdiplus::Image **img)
{
    HGLOBAL h_buffer = ::GlobalAlloc(GMEM_MOVEABLE, data.size());
    if (h_buffer)
    {
        void* p_buffer = ::GlobalLock(h_buffer);
        if (p_buffer)
        {
            CopyMemory(p_buffer, data.data(), data.size());

            IStream* p_stream = NULL;
            if (::CreateStreamOnHGlobal(h_buffer, FALSE, &p_stream) == S_OK)
            {
                *img = Gdiplus::Image::FromStream(p_stream);
                p_stream->Release();
            }

            ::GlobalUnlock(p_buffer);
        }
        ::GlobalFree(h_buffer);
    }
}

void load_image_from_resource(WORD image_id, const std::wstring &resource_section, Gdiplus::Image **img)
{
    HINSTANCE h_inst = GetModuleHandle(NULL);
    HRSRC h_resource = FindResource(h_inst, MAKEINTRESOURCE(image_id), resource_section.c_str());
    if (!h_resource)
    {
        return;
    }

    DWORD image_size = ::SizeofResource(h_inst, h_resource);
    if (!image_size)
    {
        return;
    }

    const void* resource_data = ::LockResource(::LoadResource(h_inst, h_resource));
    if (!resource_data)
    {
        return;
    }

    load_image_from_data(std::vector<uint8_t>(static_cast<const uint8_t*>(resource_data), static_cast<const uint8_t*>(resource_data) + image_size), img);
}

void load_image_from_file(std::string_view file_name, std::string_view images_path, Gdiplus::Image **img, wui::error &err)
{
    *img = Gdiplus::Image::FromFile(std::wstring(boost::nowide::widen(images_path) + L"\\" + boost::nowide::widen(file_name)).c_str());
}

void free_image(Gdiplus::Image **img)
{
    if (*img)
    {
        delete *img;
        *img = nullptr;
    }
}

namespace wui
{

#ifdef _WIN32
image::image(int32_t resource_index_, std::shared_ptr<i_theme> theme__)
    : theme_(theme__),
    position_(),
    parent_(),
    showed_(true), topmost_(false),
    file_name(),
    resource_index(resource_index_),
    img(nullptr),
    err{}
{
    load_image_from_resource(resource_index, boost::nowide::widen(theme_string(tc, tv_resource, theme_)), &img);
}
#endif

image::image(std::string_view file_name_, std::shared_ptr<i_theme> theme__)
    : theme_(theme__),
    position_(),
    parent_(),
    showed_(true), topmost_(false),
    file_name(file_name_),
#ifdef _WIN32
    resource_index(0),
#endif
    img(nullptr),
    err{}
{
    load_image_from_file(file_name_, theme_string(tc, tv_path, theme_), &img, err);
}

image::image(const std::vector<uint8_t> &data)
    : theme_(),
    position_(),
    parent_(),
    showed_(true), topmost_(false),
    file_name(),
#ifdef _WIN32
    resource_index(0),
#endif
    img(nullptr)
{
    load_image_from_data(data, &img);
}

image::~image()
{
    free_image(&img);

    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void image::draw(graphic &gr_, const rect &)
{
    if (!showed_)
    {
        return;
    }

    if (img)
    {
        Gdiplus::Graphics gr(gr_.drawable());

        auto control_pos = position();

        gr.DrawImage(
            img,
            Gdiplus::Rect(control_pos.left, control_pos.top, control_pos.width(), control_pos.height()),
            0, 0, img->GetWidth(), img->GetHeight(),
            Gdiplus::UnitPixel,
            nullptr);
    }
}

void image::set_position(const rect &position__, bool redraw)
{
    update_control_position(position_, position__, showed_ && redraw, parent_);
}

rect image::position() const
{
    return get_control_position(position_, parent_);
}

void image::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;
}

std::weak_ptr<window> image::parent() const
{
    return parent_;
}

void image::clear_parent()
{
    parent_.reset();
}

void image::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool image::topmost() const
{
    return topmost_;
}

bool image::focused() const
{
    return false;
}

bool image::focusing() const
{
    return false;
}

error image::get_error() const
{
    return err;
}

void image::update_theme_control_name(std::string_view )
{
}

void image::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

#ifdef _WIN32
    if (resource_index)
    {
        change_image(resource_index);
    }
    else
#endif
    if (!file_name.empty())
    {
        change_image(file_name);
    }
}

void image::show()
{
    showed_ = true;
    redraw();
}

void image::hide()
{
    showed_ = false;
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->redraw(position(), true);
    }
}

bool image::showed() const
{
    return showed_;
}

void image::enable()
{
}

void image::disable()
{
}

bool image::enabled() const
{
    return true;
}

void image::change_image(int32_t resource_index_)
{
    resource_index = resource_index_;

    free_image(&img);
    load_image_from_resource(resource_index, boost::nowide::widen(theme_string(tc, tv_resource, theme_)), &img);
    
    redraw();
}

void image::change_image(std::string_view file_name_)
{
    file_name = file_name_;

    free_image(&img);
    load_image_from_file(file_name, theme_string(tc, tv_path, theme_), &img, err);

    redraw();
}

void image::change_image(const std::vector<uint8_t> &data)
{
    free_image(&img);
    load_image_from_data(data, &img);

    redraw();
}

int32_t image::width() const
{
    if (img)
    {
#ifdef _WIN32
        return img->GetWidth();
#elif __linux__
        return cairo_image_surface_get_width(img);
#endif
    }
    return 0;
}

int32_t image::height() const
{
    if (img)
    {
#ifdef _WIN32
        return img->GetHeight();
#elif __linux__
        return cairo_image_surface_get_height(img);
#endif
    }
    return 0;
}

void image::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            parent__->redraw(position());
        }
    }
}

}
