
#pragma once

#include <wui/control/i_control.hpp>
#include <wui/graphic/graphic.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/color.hpp>

#include <string>
#include <functional>
#include <memory>
#include <vector>

#include <gdiplus.h>

namespace wui
{

class image : public i_control, public std::enable_shared_from_this<image>
{
public:
    image(int32_t resource_index, std::shared_ptr<i_theme> theme_ = nullptr);
    image(std::string_view file_name, std::shared_ptr<i_theme> theme_ = nullptr);
    image(const std::vector<uint8_t> &data);
    ~image();

    virtual void draw(graphic &gr, const rect &);

    virtual void set_position(const rect &position, bool redraw = true);
    virtual std::weak_ptr<window> parent() const;
    virtual rect position() const;

    virtual void set_parent(std::shared_ptr<window> window_);
    virtual void clear_parent();

    virtual void set_topmost(bool yes);
    virtual bool topmost() const;

    virtual void update_theme_control_name(std::string_view theme_control_name);
    virtual void update_theme(std::shared_ptr<i_theme> theme_ = nullptr);

    virtual void show();
    virtual void hide();
    virtual bool showed() const;

    virtual void enable();
    virtual void disable();
    virtual bool enabled() const;

    virtual bool focused() const;
    virtual bool focusing() const;

    virtual error get_error() const;

public:
    /// Image's interface
    void change_image(int32_t resource_index);

    void change_image(std::string_view file_name);
    void change_image(const std::vector<uint8_t> &data);

    int32_t width() const;
    int32_t height() const;

public:
    /// Control name in theme
    static constexpr const char *tc = "image";

    /// Used theme values
    static constexpr const char *tv_resource = "resource";
    static constexpr const char *tv_path = "path";

private:
    std::shared_ptr<i_theme> theme_;

    rect position_;

    std::weak_ptr<window> parent_;

    bool showed_, topmost_;

    std::string file_name;
	
    int32_t resource_index;
    Gdiplus::Image *img;

    error err;

    void redraw();
};

}
