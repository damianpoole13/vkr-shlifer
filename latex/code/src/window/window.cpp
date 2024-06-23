#include <wui/window/window.hpp>

#include <wui/graphic/graphic.hpp>

#include <wui/theme/theme.hpp>
#include <wui/locale/locale.hpp>

#include <wui/control/button.hpp>

#include <wui/common/flag_helpers.hpp>

#include <wui/system/tools.hpp>
#include <wui/system/wm_tools.hpp>

#include <boost/nowide/convert.hpp>

#include <algorithm>
#include <set>
#include <random>

#include <windowsx.h>

// Some helpers

void center_horizontally(wui::rect &pos, wui::system_context &context)
{
    RECT work_area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
    auto screen_width = work_area.right - work_area.left;
    pos.left = (screen_width - pos.right) / 2;
    pos.right += pos.left;
}

void center_vertically(wui::rect &pos, wui::system_context &context)
{
    RECT work_area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
    auto screen_height = work_area.bottom - work_area.top;
    pos.top = (screen_height - pos.bottom) / 2;
    pos.bottom += pos.top;
}

namespace wui
{

window::window(std::string_view theme_control_name, std::shared_ptr<i_theme> theme_)
    : context_{ 0 },
    graphic_(context_),
    controls(),
    active_control(),
    caption(),
    position_(), normal_position(),
    min_width(0), min_height(0),
    window_style_(window_style::frame),
    window_state_(window_state::normal), prev_window_state_(window_state_),
    tcn(theme_control_name),
    theme_(theme_),
    showed_(true), enabled_(true), skip_draw_(false),
    focused_index(0),
    parent_(),
    my_control_sid(), my_plain_sid(),
    transient_window(), docked_(false), docked_control(),
    subscribers_(),
    moving_mode_(moving_mode::none),
    x_click(0), y_click(0),
    err{},
    close_callback(),
    control_callback(),
    default_push_control(),
	switch_lang_button(std::make_shared<button>(locale(tcn, cl_switch_lang), std::bind(&window::switch_lang, this), button_view::image, theme_image(ti_switch_lang), 24, button::tc_tool)),
    switch_theme_button(std::make_shared<button>(locale(tcn, cl_light_theme), std::bind(&window::switch_theme, this), button_view::image, theme_image(ti_switch_theme), 24, button::tc_tool)),
    pin_button(std::make_shared<button>(locale(tcn, cl_pin), std::bind(&window::pin, this), button_view::image, theme_image(ti_pin), 24, button::tc_tool)),
    minimize_button(std::make_shared<button>("", std::bind(&window::minimize, this), button_view::image, theme_image(ti_minimize), 24, button::tc_tool)),
    expand_button(std::make_shared<button>("", [this]() { window_state_ == window_state::normal ? expand() : normal(); }, button_view::image, window_state_ == window_state::normal ? theme_image(ti_expand) : theme_image(ti_normal), 24, button::tc_tool)),
    close_button(std::make_shared<button>("", std::bind(&window::destroy, this), button_view::image, theme_image(ti_close), 24, button::tc_tool_red)),
    mouse_tracked(false)
{
	switch_lang_button->disable_focusing();
    switch_theme_button->disable_focusing();
    pin_button->disable_focusing();
    minimize_button->disable_focusing();
    expand_button->disable_focusing();
    close_button->disable_focusing();
}

window::~window()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }

    if (context_.hwnd)
    {
        DestroyWindow(context_.hwnd);
    }
}

void window::add_control(std::shared_ptr<i_control> control, const rect &control_position)
{
    if (std::find(controls.begin(), controls.end(), control) == controls.end())
    {
        control->set_parent(shared_from_this());
        control->set_position(control_position, false);
        controls.emplace_back(control);

        redraw(control->position());
    }
}

void window::remove_control(std::shared_ptr<i_control> control)
{
    if (!control)
    {
        return;
    }

    auto exists = std::find(controls.begin(), controls.end(), control);
    if (exists != controls.end())
    {
        controls.erase(exists);
    }

    if (control == docked_control)
    {
        docked_control.reset();
    }
    
    auto clear_pos = control->position();
    control->clear_parent();
    redraw(clear_pos, true);
}

void window::bring_to_front(std::shared_ptr<i_control> control)
{
    auto size = controls.size();
    if (size > 1)
    {
        auto it = std::find(controls.begin(), controls.end(), control);
        if (it != controls.end())
        {
            controls.erase(it);
        }
        controls.emplace_back(control);
    }
}

void window::move_to_back(std::shared_ptr<i_control> control)
{
    auto size = controls.size();
    if (size > 1)
    {
        auto it = std::find(controls.begin(), controls.end(), control);
        if (it != controls.end())
        {
            controls.erase(it);
        }
        controls.insert(controls.begin(), control);
    }
}

void window::redraw(const rect &redraw_position, bool clear)
{
    if (redraw_position.is_null() || skip_draw_)
    {
        return;
    }
    
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->redraw(redraw_position, clear);
    }
    else
    {
        RECT invalidatingRect = { redraw_position.left > 0 ? redraw_position.left : 0,
            redraw_position.top > 0 ? redraw_position.top : 0,
            redraw_position.right > 0 ? redraw_position.right : 0,
            redraw_position.bottom > 0 ? redraw_position.bottom : 0 };
        InvalidateRect(context_.hwnd, &invalidatingRect, clear ? TRUE : FALSE);
    }
}

std::string window::subscribe(std::function<void(const event&)> receive_callback_, event_type event_types_, std::shared_ptr<i_control> control_)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> uid(0, 61);

    auto randchar = [&uid, &gen]() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[uid(gen)];
    };

    std::string id(20, 0);
    std::generate_n(id.begin(), 20, randchar);

    subscribers_.emplace_back(event_subscriber{ id, receive_callback_, event_types_, control_ });
    return id;
}

void window::unsubscribe(std::string_view subscriber_id)
{
    auto it = std::find_if(subscribers_.begin(), subscribers_.end(), [&subscriber_id](const event_subscriber &es) {
        return es.id == subscriber_id;
    });
    if (it != subscribers_.end())
    {
        subscribers_.erase(it);
    }
}

system_context &window::context()
{
    auto parent__ = parent_.lock();
    if (!parent__)
    {
        return context_;
    }
    else
    {
        return parent__->context();
    }
}

void window::draw(graphic &gr, const rect &paint_rect)
{
    /// drawing the child window

    if (!showed_)
    {
        return;
    }

    auto window_pos = position();

    gr.draw_rect(window_pos, 
        theme_color(tcn, tv_background, theme_),
        theme_color(tcn, tv_background, theme_),
        theme_dimension(tcn, tv_border_width, theme_),
        theme_dimension(tcn, tv_round, theme_)
    );

    if (flag_is_set(window_style_, window_style::title_showed))
    {
        gr.draw_text({ window_pos.left + 10, window_pos.top + 10, 0, 0 },
            caption,
            theme_color(tcn, tv_text, theme_),
            theme_font(tcn, tv_caption_font, theme_));
    }

    std::vector<std::shared_ptr<i_control>> topmost_controls;
    
    for (auto &control : controls)
    {
        if (control->position().in(paint_rect))
        {
            if (!control->topmost())
            {
                control->draw(gr, paint_rect);
            }
            else
            {
                topmost_controls.emplace_back(control);
            }
        }
    }

    for (auto &control : topmost_controls)
    {
        control->draw(gr, paint_rect);
    }

    if (flag_is_set(window_style_, window_style::border_left) &&
        flag_is_set(window_style_, window_style::border_top) &&
        flag_is_set(window_style_, window_style::border_right) &&
        flag_is_set(window_style_, window_style::border_bottom))
    {
        gr.draw_rect(window_pos, 
            theme_color(tcn, tv_border, theme_),
            make_color(0, 0, 0, 255),
            theme_dimension(tcn, tv_border_width, theme_),
            theme_dimension(tcn, tv_round, theme_)
        );
    }
    else
    {
        draw_border(gr);
    }
}

void window::receive_control_events(const event &ev)
{
    /// Here we receive events from the parent window and relay them to our child controls

    if (!showed_)
    {
        return;
    }

    switch (ev.type)
    {
        case event_type::mouse:
            send_mouse_event(ev.mouse_event_);
        break;
        case event_type::keyboard:
        {
            auto control = get_focused();
            if (control)
            {
                send_event_to_control(control, ev);
            }
        }
        break;
        case event_type::internal:
            switch (ev.internal_event_.type)
            {
                case internal_event_type::set_focus:
                    change_focus();
                break;
                case internal_event_type::remove_focus:
                {
                    size_t focusing_controls = 0;
                    for (const auto &control : controls)
                    {
                        if (control->focused())
                        {
                            event ev;
                            ev.type = event_type::internal;
                            ev.internal_event_ = internal_event{ internal_event_type::remove_focus };
                            send_event_to_control(control, ev);

                            ++focused_index;
                        }

                        if (control->focusing())
                        {
                            ++focusing_controls;
                        }
                    }

                    if (focused_index > focusing_controls)
                    {
                        focused_index = 0;
                    }
                }
                break;
                case internal_event_type::execute_focused:
                    execute_focused();
                break;
            }
        break;
        default: break;
    }
}

void window::receive_plain_events(const event &ev)
{
    if (ev.type == event_type::internal && ev.internal_event_.type == wui::internal_event_type::size_changed)
    {
        int32_t w = ev.internal_event_.x, h = ev.internal_event_.y;
        if (docked_)
        {
            int32_t left = (w - position_.width()) / 2;
            int32_t top = (h - position_.height()) / 2;

            auto new_position = position_;
            new_position.put(left, top);
            set_position(new_position, false);
        }

        return;
    }

    send_event_to_plains(ev);
}

void window::set_position(const rect &position__, bool redraw_)
{
    auto old_position = position_;
    auto position___ = position__;

    if (context_.hwnd)
    {
        if (position___.left == -1)
        {
            center_horizontally(position___, context_);
        }
        if (position___.top == -1)
        {
            center_vertically(position___, context_);
        }

        position_ = position___;

    SetWindowPos(context_.hwnd, NULL, position___.left, position___.top, position___.width(), position___.height(), NULL);
        redraw({ 0, 0, position___.width(), position___.height() }, true);
    }

    auto parent__ = parent_.lock();
    if (parent__)
    {
        auto parent_pos = parent__->position();

        auto left = position___.left;
        if (left == -1)
        {
            left = (parent_pos.width() - position___.width()) / 2;
        }
        auto top = position___.top;
        if (top == -1)
        {
            top = (parent_pos.height() - position___.height()) / 2;
        }

        position_ = { left, top, left + position___.width(), top + position___.height() };

        skip_draw_ = true;
        send_internal(internal_event_type::size_changed, position_.width(), position_.height());

        if (old_position.width() != position_.width())
        {
            update_buttons();
        }
        skip_draw_ = false;

        if (showed_ && redraw_)
        {
            rect update_field {
                old_position.left < position_.left ? old_position.left : position_.left,
                old_position.top < position_.top ? old_position.top : position_.top,
                old_position.right > position_.right ? old_position.right : position_.right,
                old_position.bottom > position_.bottom ? old_position.bottom : position_.bottom
            };
            parent__->redraw(update_field, true);
        }
    }
}

rect window::position() const
{
    return get_control_position(position_, parent_);
}

void window::set_parent(std::shared_ptr<window> window)
{
    parent_ = window;

    if (window)
    {
        if (context_.hwnd)
        {
            DestroyWindow(context_.hwnd);
        }

        my_control_sid = window->subscribe(std::bind(&window::receive_control_events, this, std::placeholders::_1), event_type::all, shared_from_this());
        my_plain_sid = window->subscribe(std::bind(&window::receive_plain_events, this, std::placeholders::_1), event_type::all);

        pin_button->set_caption(locale(tcn, cl_unpin));
    }
}

std::weak_ptr<window> window::parent() const
{
    return parent_;
}

void window::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->unsubscribe(my_control_sid);
        parent__->unsubscribe(my_plain_sid);
    }

    parent_.reset();
}

void window::set_topmost(bool yes)
{
    if (yes)
    {
        set_style(static_cast<window_style>(static_cast<uint32_t>(window_style_) | static_cast<uint32_t>(window_style::topmost)));
    }
    else
    {
        set_style(static_cast<window_style>(static_cast<uint32_t>(window_style_) & ~static_cast<uint32_t>(window_style::topmost)));
    }
}

bool window::topmost() const
{
    return docked_ || parent_.lock() || flag_is_set(window_style_, window_style::topmost);
}

bool window::focused() const
{
    for (const auto &control : controls)
    {
        if (control->focused())
        {
            return true;
        }
    }

    return false;
}

bool window::focusing() const
{
    for (const auto &control : controls)
    {
        if (control->focusing())
        {
            return true;
        }
    }

    return false;
}

void window::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void window::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    if (context_.valid() && !parent_.lock())
    {
        graphic_.set_background_color(theme_color(tcn, tv_background, theme_));

        RECT client_rect;
        GetClientRect(context_.hwnd, &client_rect);
        InvalidateRect(context_.hwnd, &client_rect, TRUE);
    }

    for (auto &control : controls)
    {
        control->update_theme(theme_);
    }

    update_button_images();
}

void window::show()
{
    showed_ = true;

    if (!parent_.lock())
    {
        ShowWindow(context_.hwnd, SW_SHOW);
    }
    else
    {
        for (auto &control : controls)
        {
            control->show();
        }
    }
}

void window::hide()
{
    showed_ = false;

    auto parent__ = parent_.lock();
    if (!parent__)
    {
        ShowWindow(context_.hwnd, SW_HIDE);
    }
    else
    {
        for (auto &control : controls)
        {
            control->hide();
        }

        parent__->redraw(position(), true);
    }
}

bool window::showed() const
{
    return showed_;
}

void window::enable()
{
    enabled_ = true;

    EnableWindow(context_.hwnd, TRUE);
    SetWindowPos(context_.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(context_.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
}

void window::disable()
{
    enabled_ = false;

    EnableWindow(context_.hwnd, FALSE);
}

bool window::enabled() const
{
    return enabled_;
}

error window::get_error() const
{
    return err;
}

void window::switch_lang()
{
	if (control_callback)
	{
		std::string tooltip_text;
		bool continue_ = true;
		control_callback(window_control::lang, tooltip_text, continue_);
		switch_lang_button->set_caption(tooltip_text);
	}
}

void window::switch_theme()
{
    if (control_callback)
    {
        std::string tooltip_text;
        bool continue_ = true;
        control_callback(window_control::theme, tooltip_text, continue_);
        switch_theme_button->set_caption(tooltip_text);
    }
}

void window::pin()
{
    if (active_control)
    {
        mouse_event me{ mouse_event_type::leave };
        send_event_to_control(active_control, { event_type::mouse, me });
        active_control.reset();
    }

    if (control_callback)
    {
        std::string tooltip_text;
        bool continue_ = true;
        control_callback(window_control::pin, tooltip_text, continue_);
        pin_button->set_caption(tooltip_text);
    }
}

void window::minimize()
{
    if (window_state_ == window_state::minimized)
    {
        return;
    }

    if (control_callback)
    {
        std::string text = "minimize";
        bool continue_ = true;
        control_callback(window_control::state, text, continue_);
        if (!continue_)
        {
            return;
        }
    }

    normal_position = position();

    prev_window_state_ = window_state_;

    ShowWindow(context_.hwnd, SW_MINIMIZE);

    window_state_ = window_state::minimized;

    send_internal(internal_event_type::window_minimized, -1, -1);
}

void window::expand()
{
    if (window_state_ == window_state::maximized)
    {
        return;
    }

    if (control_callback)
    {
        std::string text = "expand";
        bool continue_ = true;
        control_callback(window_control::state, text, continue_);
        if (!continue_)
        {
            return;
        }
    }

    window_state_ = window_state::maximized;
    auto screenSize = wui::get_screen_size(context());
    if (position().width() != screenSize.width() && position().height() != screenSize.height())
    {
        normal_position = position();
    }

    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfo(MonitorFromWindow(context_.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
    {
        if (flag_is_set(window_style_, window_style::title_showed) && flag_is_set<DWORD>(mi.dwFlags, MONITORINFOF_PRIMARY)) // normal window maximization
        {
            RECT work_area;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
            SetWindowPos(context_.hwnd, NULL, work_area.left, work_area.top, work_area.right, work_area.bottom, NULL);
        }
        else // expand to full screen
        {
            SetWindowPos(context_.hwnd,
                HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    expand_button->set_image(theme_image(ti_normal, theme_));
}

void window::normal()
{
    if (control_callback)
    {
        std::string text = "normal";
        bool continue_ = true;
        control_callback(window_control::state, text, continue_);
        if (!continue_)
        {
            return;
        }
    }

    SetWindowPos(context_.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(context_.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
    
    ShowWindow(context_.hwnd, SW_RESTORE);

    window_state_ = window_state::normal;

    if (!normal_position.is_null())
    {
        set_position(normal_position, false);
    }

    expand_button->set_image(theme_image(ti_expand, theme_));

    update_buttons();

    send_internal(internal_event_type::window_normalized, position_.width(), position_.height()); 

    redraw({ 0, 0, position_.width(), position_.height() }, true);
}

window_state window::state() const
{
    return window_state_;
}

#ifndef _WIN32
void set_wm_name(system_context &context_, std::string_view caption)
{
    xcb_icccm_set_wm_name(context_.connection, context_.wnd,
        XCB_ATOM_STRING, 8,
        caption.size(), caption.data());

    xcb_icccm_set_wm_icon_name(context_.connection, context_.wnd,
        XCB_ATOM_STRING, 8,
        caption.size(), caption.data());

    std::string class_hint = std::string(caption) + '\0' + std::string(caption) + '\0';
    xcb_icccm_set_wm_class(context_.connection, context_.wnd, class_hint.size(), class_hint.c_str());
}
#endif

void window::set_caption(std::string_view caption_)
{
    caption = caption_;

    if (flag_is_set(window_style_, window_style::title_showed) && !parent_.lock())
    {
        SetWindowText(context_.hwnd, boost::nowide::widen(caption).c_str());

        redraw({ 0, 0, position_.width(), 30 }, true);
    }
}

void window::set_style(window_style style)
{
    window_style_ = style;

    update_buttons();

    if (topmost())
    {
        SetWindowPos(context_.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        if (window_state_ == window_state::maximized)
        {
            MONITORINFO mi = { sizeof(mi) };
            if (GetMonitorInfo(MonitorFromWindow(context_.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
            {
                SetWindowPos(context_.hwnd,
                    HWND_TOP,
                    mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            }
        }
    }
    else
    {
        SetWindowPos(context_.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void window::set_min_size(int32_t width, int32_t height)
{
    min_width = width;
    min_height = height;
}

void window::set_transient_for(std::shared_ptr<window> window_, bool docked__)
{
    transient_window = window_;
    docked_ = docked__;
}

void window::start_docking(std::shared_ptr<i_control> control)
{
    enabled_ = false;

    docked_control = control;

    send_internal(internal_event_type::remove_focus, 0, 0);

    set_focused(control);
}

void window::end_docking()
{
    enabled_ = true;

    docked_control.reset();
}

void window::disable_draw()
{
    skip_draw_ = true;
}

void window::enable_draw()
{
    skip_draw_ = false;
}

bool window::draw_enabled() const
{
    return !skip_draw_;
}

void window::emit_event(int32_t x, int32_t y)
{
    auto parent__ = parent_.lock();
    if (!parent__)
    {
        PostMessage(context_.hwnd, WM_USER, x, y);
    }
    else
    {
        parent__->emit_event(x, y);
    }
}

void window::set_control_callback(std::function<void(window_control control, std::string &text, bool &continue_)> callback_)
{
    control_callback = callback_;
}

void window::set_default_push_control(std::shared_ptr<i_control> control)
{
    default_push_control = control;
}

void window::send_event_to_control(const std::shared_ptr<i_control> &control_, const event &ev)
{
    auto it = std::find_if(subscribers_.begin(), subscribers_.end(), [control_, ev](const event_subscriber &es) {
        return flag_is_set(es.event_types, ev.type) && es.control == control_;
    });
    
    if (it != subscribers_.end())
    {
        it->receive_callback(ev);
    }
}

void window::send_event_to_plains(const event &ev)
{
    auto subscribers__ = subscribers_; // This is necessary to be able to remove the subscriber in the callback
    for (auto &s : subscribers__)
    {
        if (!s.control && flag_is_set(s.event_types, ev.type) && s.receive_callback)
        {
            s.receive_callback(ev);
        }
    }
}

void window::send_mouse_event(const mouse_event &ev)
{
    if (!enabled_ && !docked_control)
    {
        return;
    }

    if (active_control && !active_control->position().in(ev.x, ev.y))
    {
        mouse_event me{ mouse_event_type::leave };
        send_event_to_control(active_control, { event_type::mouse, me });

        active_control.reset();
    }

    send_event_to_plains({ event_type::mouse, ev });

    auto send_mouse_event_to_control = [this](std::shared_ptr<wui::i_control> &send_to_control,
        const mouse_event &ev_) noexcept -> void
    {
        if (active_control == send_to_control)
        {
            if (send_to_control->focusing() && ev_.type == mouse_event_type::left_down)
            {
                set_focused(send_to_control);
            }

            return send_event_to_control(send_to_control, { event_type::mouse, ev_ });
        }
        else
        {
            if (active_control)
            {
                mouse_event me{ mouse_event_type::leave };
                send_event_to_control(active_control, { event_type::mouse, me });
            }

            if (ev_.y < 5 || (ev_.y < 24 && ev_.x > position().width() - 5)) /// control buttons border
            {
                return active_control.reset();
            }
            else
            {
                active_control = send_to_control;

                mouse_event me{ mouse_event_type::enter };
                return send_event_to_control(send_to_control, { event_type::mouse, me });
            }
        }
    };

    if (enabled_)
    {
        auto end = controls.rend();
        for (auto control = controls.rbegin(); control != end; ++control)
        {
            if (*control && (*control)->topmost() && (*control)->showed() && (*control)->position().in(ev.x, ev.y))
            {
                return send_mouse_event_to_control(*control, ev);
            }
        }

        for (auto control = controls.rbegin(); control != end; ++control)
        {
            if (*control && (*control)->showed() && (*control)->position().in(ev.x, ev.y))
            {
                return send_mouse_event_to_control(*control, ev);
            }
        }
    }
    else
    {
        for (auto &control : controls)
        {
            if (control && control->position().in(ev.x, ev.y) && control == docked_control)
            {
                return send_mouse_event_to_control(control, ev);
            }
        }
    }
}

bool window::check_control_here(int32_t x, int32_t y)
{
    for (auto &control : controls)
    {
        if (control->showed() &&
            control->position().in(x, y) &&
            std::find_if(subscribers_.begin(), subscribers_.end(), [&control](const event_subscriber &es) { return es.control == control; }) != subscribers_.end())
        {
            return true;
        }
    }
    return false;
}

void window::change_focus()
{
    if (controls.empty())
    {
        return;
    }

    for (auto &control : controls)
    {
        if (control->focused() && control != docked_control)
        {
            event ev;
            ev.type = event_type::internal;
            ev.internal_event_ = internal_event{ internal_event_type::remove_focus };
            send_event_to_control(control, ev);

            if (!control->focused()) /// need to change the focus inside the internal elements of the control
            {
                ++focused_index;
            }
            else
            {
                return;
            }
            break;
        }
    }

    size_t focusing_controls = 0;
    for (const auto &control : controls)
    {
        if (control->focusing())
        {
            ++focusing_controls;
        }
    }

    if (focused_index >= focusing_controls)
    {
        focused_index = 0;
    }

    set_focused(focused_index);
}

void window::execute_focused()
{
    std::shared_ptr<wui::i_control> control;

    if (docked_control)
    {
        control = docked_control;
    }
    else if (default_push_control)
    {
        control = default_push_control;
    }
    else
    {
        control = get_focused();
    }

    if (control)
    {
        event ev;
        ev.type = event_type::internal;
        ev.internal_event_ = internal_event{ internal_event_type::execute_focused };

        send_event_to_control(control, ev);
    }
}

void window::set_focused(std::shared_ptr<i_control> control)
{
    size_t index = 0;
    for (auto &c : controls)
    {
        if (c == control)
        {
            if (c->focused())
            {
                return;
            }
            focused_index = index;
        }

        if (c->focused())
        {
            event ev;
            ev.type = event_type::internal;
            ev.internal_event_ = internal_event{ internal_event_type::remove_focus };
            send_event_to_control(c, ev);
        }

        if (c->focusing())
        {
            ++index;
        }
    }

    if (control)
    {
        event ev;
        ev.type = event_type::internal;
        ev.internal_event_ = internal_event{ internal_event_type::set_focus };
        send_event_to_control(control, ev);
    }
}

void window::set_focused(size_t focused_index_)
{
    size_t index = 0;
    for (auto &control : controls)
    {
        if (control->focusing())
        {
            if (index == focused_index_)
            {
                event ev;
                ev.type = event_type::internal;
                ev.internal_event_ = internal_event{ internal_event_type::set_focus };
                send_event_to_control(control, ev);

                break;
            }

            ++index;
        }
    }
}

std::shared_ptr<i_control> window::get_focused()
{
    for (auto &control : controls)
    {
        if (control->focused())
        {
            return control;
        }
    }

    return nullptr;
}

void window::update_button_images()
{
	switch_lang_button->set_image(theme_image(ti_switch_lang, theme_));
    switch_theme_button->set_image(theme_image(ti_switch_theme, theme_));
    pin_button->set_image(theme_image(ti_pin, theme_));
    minimize_button->set_image(theme_image(ti_minimize, theme_));
    expand_button->set_image(theme_image(window_state_ == window_state::normal ? ti_expand : ti_normal, theme_));
    close_button->set_image(theme_image(ti_close, theme_));
}

void window::update_buttons()
{
	auto border_height = flag_is_set(window_style_, window_style::border_top) ? theme_dimension(tcn, tv_border_width, theme_) : 0;
    auto border_width = flag_is_set(window_style_, window_style::border_right) ? theme_dimension(tcn, tv_border_width, theme_) : 0;

    auto btn_size = 28;
    auto left = position_.width() - btn_size - border_width;
    auto top = border_height;

    if (flag_is_set(window_style_, window_style::close_button))
    {
        close_button->set_position({ left, top, left + btn_size, top + btn_size }, false);
        close_button->show();

        left -= btn_size;
    }
    else
    {
        close_button->hide();
    }

    if (flag_is_set(window_style_, window_style::expand_button) || flag_is_set(window_style_, window_style::minimize_button))
    {
        expand_button->set_position({ left, top, left + btn_size, top + btn_size }, false);
        expand_button->show();

        left -= btn_size;

        if (flag_is_set(window_style_, window_style::expand_button))
        {
            expand_button->enable();
        }
        else
        {
            expand_button->disable();
        }
    }
    else
    {
        expand_button->hide();
    }

    if (flag_is_set(window_style_, window_style::minimize_button))
    {
        minimize_button->set_position({ left, top, left + btn_size, top + btn_size }, false);
        minimize_button->show();

        left -= btn_size;
    }
    else
    {
        minimize_button->hide();
    }

    if (flag_is_set(window_style_, window_style::pin_button))
    {
        pin_button->set_position({ left, top, left + btn_size, top + btn_size }, false);
        pin_button->show();

        left -= btn_size;
    }
    else
    {
        pin_button->hide();
    }

    if (flag_is_set(window_style_, window_style::switch_theme_button))
    {
        switch_theme_button->set_position({ left, top, left + btn_size, top + btn_size }, false);
        switch_theme_button->show();

        left -= btn_size;
    }
    else
    {
        switch_theme_button->hide();
    }

	if (flag_is_set(window_style_, window_style::switch_lang_button))
	{
		switch_lang_button->set_position({ left, top, left + btn_size, top + btn_size }, false);
		switch_lang_button->show();

		left -= btn_size;
	}
	else
	{
		switch_lang_button->hide();
	}
}

void window::draw_border(graphic &gr)
{
    auto c = theme_color(tcn, tv_border, theme_);
    auto x = theme_dimension(tcn, tv_border_width, theme_);

    int32_t l = 0, t = 0, w = 0, h = 0;

    auto pos = position();

    if (parent_.lock())
    {
        l = pos.left;
        t = pos.top;
        w = pos.right - x;
        h = pos.bottom - x;

    }
    else
    {
        w = position_.width() - x;
        h = position_.height() - x;
    }

    if (flag_is_set(window_style_, window_style::border_left))
    {
        gr.draw_line({ l, t, l, h }, c, x);
    }
    if (flag_is_set(window_style_, window_style::border_top))
    {
        gr.draw_line({ l, t, w, t }, c, x);
    }
    if (flag_is_set(window_style_, window_style::border_right))
    {
        gr.draw_line({ w, t, w, h }, c, x);
    }
    if (flag_is_set(window_style_, window_style::border_bottom))
    {
        gr.draw_line({ l, h, w, h }, c, x);
    }
}

void window::send_internal(internal_event_type type, int32_t x, int32_t y)
{
    event ev_;
    ev_.type = event_type::internal;
    ev_.internal_event_ = internal_event{ type, x, y };
    send_event_to_plains(ev_);
}

void window::send_system(system_event_type type, int32_t x, int32_t y)
{
    event ev_;
    ev_.type = event_type::system;
    ev_.system_event_ = system_event{ type, x, y };
    send_event_to_plains(ev_);
}

std::shared_ptr<window> window::get_transient_window()
{
    return transient_window.lock();
}

bool window::init(std::string_view caption_, const rect &position__, window_style style, std::function<void(void)> close_callback_)
{
    err.reset();

    caption = caption_;

    if (!position__.is_null())
    {
        position_ = position__;
    }

    window_style_ = style;
    close_callback = close_callback_;

	add_control(switch_lang_button, { 0 });
    add_control(switch_theme_button, { 0 });
    add_control(pin_button, { 0 });
    add_control(minimize_button, { 0 });
    add_control(expand_button, { 0 });
    add_control(close_button, { 0 });

    update_button_images();
    update_buttons();

    auto transient_window_ = get_transient_window();
    if (transient_window_)
    {
        if (transient_window_->window_state_ == window_state::minimized)
        {
            transient_window_->normal();
        }

        if (docked_ && transient_window_->position_ > position_)
        {
			int32_t left = (transient_window_->position().width() - position_.width()) / 2;
            int32_t top = (transient_window_->position().height() - position_.height()) / 2;

			transient_window_->add_control(shared_from_this(), { left, top, left + position_.width(), top + position_.height() });
            transient_window_->start_docking(shared_from_this());
        }
        else
        {
            auto transient_window_pos = transient_window_->position();

            int32_t left = 0, top = 0;

            if (transient_window_pos.width() > 0 && transient_window_pos.height() > 0)
            {
                left = transient_window_pos.left + ((transient_window_pos.width() - position_.width()) / 2);
                top = transient_window_pos.top + ((transient_window_pos.height() - position_.height()) / 2);
            }
            else
            {
                RECT work_area;
                SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
                auto screen_width = work_area.right - work_area.left,
                    screen_height = work_area.bottom - work_area.top;
                left = (screen_width - position_.width()) / 2;
                top = (screen_height - position_.height()) / 2;
            }

            position_.put(left, top);

            transient_window_->disable();
        }
    }

    auto parent__ = parent_.lock();
    if (parent__)
    {
        send_internal(internal_event_type::window_created, 0, 0);

        send_internal(internal_event_type::size_changed, position_.width(), position_.height());

        parent__->redraw(position());

        return true;
    }

    auto h_inst = GetModuleHandle(NULL);

    WNDCLASSEXW wcex = { 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_DBLCLKS;
    wcex.lpfnWndProc = window::wnd_proc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(*this);
    wcex.hInstance = h_inst;
    wcex.hbrBackground = NULL;
    wcex.lpszClassName = L"WUI Window";

    RegisterClassExW(&wcex);

    if (position_.left == -1)
    {
        center_horizontally(position_, context_);
    }
    if (position_.top == -1)
    {
        center_vertically(position_, context_);
    }
    
    context_.hwnd = CreateWindowEx(!topmost() ? 0 : WS_EX_TOPMOST,
        wcex.lpszClassName,
        L"",
        WS_VISIBLE | WS_MINIMIZEBOX | WS_POPUP | (window_state_ == window_state::minimized ? WS_MINIMIZE : 0),
        position_.left,
        position_.top,
        position_.width(),
        position_.height(),
        nullptr,
        nullptr,
        h_inst,
        this);

    if (!context_.hwnd)
    {
        return false;
    }

    if (window_state_ == window_state::maximized)
    {
        expand();
    }

    SetWindowText(context_.hwnd, boost::nowide::widen(caption).c_str());

    UpdateWindow(context_.hwnd);

    send_internal(internal_event_type::size_changed, position_.width(), position_.height());

    if (!showed_)
    {
        ShowWindow(context_.hwnd, SW_HIDE);
    }

    return true;
}

void window::destroy()
{
    if (control_callback)
    {
        std::string text = "close";
        bool continue_= true;
        control_callback(window_control::close, text, continue_);
        if (!continue_)
        {
            return;
        }
    }

    std::vector<std::shared_ptr<i_control>> controls_;
    controls_ = controls; /// This is necessary to solve the problem of removing child controls within a control

    for (auto &control : controls_)
    {
        if (control)
        {
            control->clear_parent();
        }
    }

    if (active_control)
    {
        mouse_event me{ mouse_event_type::leave };
        send_event_to_control(active_control, { event_type::mouse, me });
    }

    active_control.reset();

    controls.clear();

    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());

        auto transient_window_ = get_transient_window();
        if (transient_window_)
        {
            transient_window_->end_docking();
        }

        if (close_callback)
        {
            close_callback();
        }
    }
    else
    {
        DestroyWindow(context_.hwnd);
    }
}

/// Windows specified code

uint8_t get_key_modifier()
{
    if (GetKeyState(VK_SHIFT) < 0)
    {
        return vk_lshift;
    }
    else if (GetKeyState(VK_CAPITAL) & 0x0001)
    {
        return vk_capital;
    }
    else if (GetKeyState(VK_MENU) < 0)
    {
        return vk_alt;
    }
    else if (GetKeyState(VK_LCONTROL) < 0)
    {
        return vk_lcontrol;
    }
    else if (GetKeyState(VK_RCONTROL) < 0)
    {
        return vk_rcontrol;
    }
    else if (GetKeyState(VK_INSERT) & 0x0001)
    {
        return vk_insert;
    }
    else if (GetKeyState(VK_NUMLOCK) & 0x0001)
    {
        return vk_numlock;
    }
    return 0;
}

LRESULT CALLBACK window::wnd_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
        case WM_CREATE:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCT*>(l_param)->lpCreateParams));

            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            wnd->graphic_.init(get_screen_size(wnd->context_), theme_color(wnd->tcn, tv_background, wnd->theme_));

            wnd->send_internal(internal_event_type::window_created, 0, 0);
        }
        break;
        case WM_PAINT:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            PAINTSTRUCT ps;
            auto bpdc = BeginPaint(hwnd, &ps);

            if (bpdc == NULL)
            {
                return 0;
            }

            const rect paint_rect{ ps.rcPaint.left,
                ps.rcPaint.top,
                ps.rcPaint.right,
                ps.rcPaint.bottom };

            if (ps.fErase)
            {
                wnd->graphic_.clear(paint_rect);
            }
            if (flag_is_set(wnd->window_style_, window_style::title_showed) && !wnd->parent_.lock())
            {
                auto caption_font = theme_font(wnd->tcn, tv_caption_font, wnd->theme_);

                auto caption_rect = wnd->graphic_.measure_text(wnd->caption, caption_font);
                caption_rect.move(5, 5);

                if (caption_rect.in(paint_rect))
                {
                    wnd->graphic_.draw_rect(caption_rect, theme_color(wnd->tcn, tv_background, wnd->theme_));
                    wnd->graphic_.draw_text(caption_rect,
                        wnd->caption,
                        theme_color(wnd->tcn, tv_text, wnd->theme_),
                        caption_font);
                }
            }

            wnd->draw_border(wnd->graphic_);

            std::vector<std::shared_ptr<i_control>> topmost_controls;

            for (auto &control : wnd->controls)
            {
                if (control->position().in(paint_rect))
                {
                    if (!control->topmost())
                    {
                        control->draw(wnd->graphic_, paint_rect);
                    }
                    else
                    {
                        topmost_controls.emplace_back(control);
                    }
                }
            }

            for (auto &control : topmost_controls)
            {
                control->draw(wnd->graphic_, paint_rect);
            }

            wnd->graphic_.flush(paint_rect);

            EndPaint(hwnd, &ps);
        }
        break;
        case WM_MOUSEMOVE:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            RECT window_rect;
            GetWindowRect(hwnd, &window_rect);

            int16_t x_mouse = GET_X_LPARAM(l_param);
            int16_t y_mouse = GET_Y_LPARAM(l_param);

            static bool cursor_size_view = false;

            if (flag_is_set(wnd->window_style_, window_style::resizable) && wnd->window_state_ == window_state::normal)
            {
                if ((x_mouse > window_rect.right - window_rect.left - 5 && y_mouse > window_rect.bottom - window_rect.top - 5) ||
                    (x_mouse < 5 && y_mouse < 5))
                {
                    set_cursor(wnd->context_, cursor::size_nwse);
                    cursor_size_view = true;
                }
                else if ((x_mouse > window_rect.right - window_rect.left - 5 && y_mouse < 5) ||
                    (x_mouse < 5 && y_mouse > window_rect.bottom - window_rect.top - 5))
                {
                    set_cursor(wnd->context_, cursor::size_nesw);
                    cursor_size_view = true;
                }
                else if (x_mouse > window_rect.right - window_rect.left - 5 || x_mouse < 5)
                {
                    set_cursor(wnd->context_, cursor::size_we);
                    cursor_size_view = true;
                }
                else if (y_mouse > window_rect.bottom - window_rect.top - 5 || y_mouse < 5)
                {
                    set_cursor(wnd->context_, cursor::size_ns);
                    cursor_size_view = true;
                }
                else if (cursor_size_view &&
                    x_mouse > 5 && x_mouse < window_rect.right - window_rect.left - 5 &&
                    y_mouse > 5 && y_mouse < window_rect.bottom - window_rect.top - 5)
                {
                    set_cursor(wnd->context_, cursor::default_);
                    cursor_size_view = false;
                }
            }

            if (!wnd->mouse_tracked)
            {
                TRACKMOUSEEVENT track_mouse_event;

                track_mouse_event.cbSize = sizeof(track_mouse_event);
                track_mouse_event.dwFlags = TME_LEAVE;
                track_mouse_event.hwndTrack = hwnd;

                TrackMouseEvent(&track_mouse_event);

                wnd->mouse_tracked = true;
            }

            if (wnd->moving_mode_ != moving_mode::none)
            {
                switch (wnd->moving_mode_)
                {
                    case moving_mode::move:
                    {
                        int32_t x_window = window_rect.left + x_mouse - wnd->x_click;
                        int32_t y_window = window_rect.top + y_mouse - wnd->y_click;

                        SetWindowPos(hwnd, NULL, x_window, y_window, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    break;
                    case moving_mode::size_we_left:
                    {
                        POINT scr_mouse = { 0 };
                        GetCursorPos(&scr_mouse);

                        int32_t width = window_rect.right - window_rect.left - x_mouse;
                        int32_t height = window_rect.bottom - window_rect.top;

                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, scr_mouse.x, window_rect.top, width, height, SWP_NOZORDER);
                        }
                    }
                    break;
                    case moving_mode::size_we_right:
                    {
                        int32_t width = x_mouse;
                        int32_t height = window_rect.bottom - window_rect.top;
                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
                        }
                    }
                    break;
                    case moving_mode::size_ns_top:
                    {
                        POINT scr_mouse = { 0 };
                        GetCursorPos(&scr_mouse);

                        int32_t width = window_rect.right - window_rect.left;
                        int32_t height = window_rect.bottom - window_rect.top - y_mouse;
                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, window_rect.left, scr_mouse.y, width, height, SWP_NOZORDER);
                        }
                    }
                    break;
                    case moving_mode::size_ns_bottom:
                    {
                        int32_t width = window_rect.right - window_rect.left;
                        int32_t height = y_mouse;
                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
                        }
                    }
                    break;
                    case moving_mode::size_nesw_top:
                    {
                        POINT scr_mouse = { 0 };
                        GetCursorPos(&scr_mouse);

                        int32_t width = x_mouse;
                        int32_t height = window_rect.bottom - window_rect.top - y_mouse;
                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, window_rect.left, scr_mouse.y, width, height, SWP_NOZORDER);
                        }
                    }
                    break;
                    case moving_mode::size_nwse_bottom:
                    {
                        int32_t width = x_mouse;
                        int32_t height = y_mouse;
                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
                        }
                    }
                    break;
                    case moving_mode::size_nwse_top:
                    {
                        POINT scrMouse = { 0 };
                        GetCursorPos(&scrMouse);

                        int32_t width = window_rect.right - window_rect.left - x_mouse;
                        int32_t height = window_rect.bottom - window_rect.top - y_mouse;
                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, scrMouse.x, scrMouse.y, width, height, SWP_NOZORDER);
                        }
                    }
                    break;
                    case moving_mode::size_nesw_bottom:
                    {
                        POINT scr_mouse = { 0 };
                        GetCursorPos(&scr_mouse);

                        int32_t width = window_rect.right - window_rect.left - x_mouse;
                        int32_t height = y_mouse;
                        if (width > wnd->min_width && height > wnd->min_height)
                        {
                            SetWindowPos(hwnd, NULL, scr_mouse.x, window_rect.top, width, height, SWP_NOZORDER);
                        }
                    }
                    break;
                }
            }
            else
            {
                wnd->send_mouse_event({ mouse_event_type::move, x_mouse, y_mouse });
            }
        }
        break;
        case WM_LBUTTONDOWN:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            SetCapture(hwnd);

            RECT window_rect;
            GetWindowRect(hwnd, &window_rect);

            wnd->x_click = GET_X_LPARAM(l_param);
            wnd->y_click = GET_Y_LPARAM(l_param);

            wnd->send_mouse_event({ mouse_event_type::left_down, wnd->x_click, wnd->y_click });

            if (wnd->window_state_ == window_state::normal)
            {
                if (flag_is_set(wnd->window_style_, window_style::moving) &&
                    !wnd->check_control_here(wnd->x_click, wnd->y_click))
                {
                    wnd->moving_mode_ = moving_mode::move;
                }

                if (flag_is_set(wnd->window_style_, window_style::resizable))
                {
                    if (wnd->x_click > window_rect.right - window_rect.left - 5 && wnd->y_click > window_rect.bottom - window_rect.top - 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_nwse_bottom;
                    }
                    else if (wnd->x_click < 5 && wnd->y_click < 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_nwse_top;
                    }
                    else if (wnd->x_click > window_rect.right - window_rect.left - 5 && wnd->y_click < 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_nesw_top;
                    }
                    else if (wnd->x_click < 5 && wnd->y_click > window_rect.bottom - window_rect.top - 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_nesw_bottom;
                    }
                    else if (wnd->x_click > window_rect.right - window_rect.left - 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_we_right;
                    }
                    else if (wnd->x_click < 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_we_left;
                    }
                    else if (wnd->y_click > window_rect.bottom - window_rect.top - 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_ns_bottom;
                    }
                    else if (wnd->y_click < 5)
                    {
                        wnd->moving_mode_ = moving_mode::size_ns_top;
                    }
                }
            }
        }
        break;
        case WM_LBUTTONUP:
        {
            ReleaseCapture();

            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            wnd->moving_mode_ = moving_mode::none;

            wnd->send_mouse_event({ mouse_event_type::left_up, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) });
        }
        break;
        case WM_RBUTTONDOWN:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            wnd->send_mouse_event({ mouse_event_type::right_down, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) });
        }
        break;
        case WM_RBUTTONUP:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            wnd->send_mouse_event({ mouse_event_type::right_up, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) });
        }
        break;
        case WM_LBUTTONDBLCLK:
        {
            ReleaseCapture();

            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            wnd->moving_mode_ = moving_mode::none;

            wnd->send_mouse_event({ mouse_event_type::left_double, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) });
        }
        break;
        case WM_MOUSELEAVE:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            wnd->mouse_tracked = false;
            wnd->send_mouse_event({ mouse_event_type::leave });
        }
        break;
        case WM_MOUSEWHEEL:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            POINT p = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            ScreenToClient(hwnd, &p);
            wnd->send_mouse_event({ mouse_event_type::wheel, p.x, p.y, GET_WHEEL_DELTA_WPARAM(w_param) });
        }
        break;
        case WM_SIZE:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            auto width = LOWORD(l_param), height = HIWORD(l_param);

            wnd->position_ = { wnd->position_.left, wnd->position_.top, wnd->position_.left + width, wnd->position_.top + height };

            wnd->update_buttons();

            wnd->send_internal(wnd->window_state_ != window_state::maximized ? internal_event_type::size_changed : internal_event_type::window_expanded, width, height);

            RECT invalidatingRect = { 0, 0, width, height };
            InvalidateRect(hwnd, &invalidatingRect, FALSE);
        }
        break;
        case WM_MOVE:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            RECT window_rect = { 0 };
            GetWindowRect(hwnd, &window_rect);
            wnd->position_ = rect{ window_rect.left, window_rect.top, window_rect.right, window_rect.bottom };

            wnd->send_internal(internal_event_type::position_changed, window_rect.left, window_rect.top);
        }
        break;
        case WM_SYSCOMMAND:
            if (w_param == SC_RESTORE)
            {
                window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                wnd->window_state_ = wnd->prev_window_state_;
            }
            return DefWindowProc(hwnd, message, w_param, l_param);
        break;
        case WM_KEYDOWN:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            if (w_param == VK_TAB)
            {
                wnd->change_focus();
            }
            else if (w_param == VK_RETURN)
            {
                wnd->execute_focused();
            }

            event ev;
            ev.type = event_type::keyboard;
            ev.keyboard_event_ = keyboard_event{ keyboard_event_type::down, get_key_modifier(), 0 };
            ev.keyboard_event_.key[0] = static_cast<uint8_t>(w_param);

            auto control = wnd->get_focused();
            if (control)
            {
                wnd->send_event_to_control(control, ev);
            }
            wnd->send_event_to_plains(ev);
        }
        break;
        case WM_KEYUP:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            event ev;
            ev.type = event_type::keyboard;
            ev.keyboard_event_ = keyboard_event{ keyboard_event_type::up, get_key_modifier(), 0 };
            ev.keyboard_event_.key[0] = static_cast<uint8_t>(w_param);

            auto control = wnd->get_focused();
            if (control)
            {
                wnd->send_event_to_control(control, ev);
            }
            wnd->send_event_to_plains(ev);
        }
        break;
        case WM_CHAR:
            if (w_param != VK_ESCAPE && w_param != VK_BACK)
            {
                window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

                event ev;
                ev.type = event_type::keyboard;
                ev.keyboard_event_ = keyboard_event{ keyboard_event_type::key, get_key_modifier(), 0 };
                auto narrow_str = boost::nowide::narrow(reinterpret_cast<const wchar_t*>(&w_param));
                memcpy(ev.keyboard_event_.key, narrow_str.c_str(), narrow_str.size());
                ev.keyboard_event_.key_size = static_cast<uint8_t>(narrow_str.size());

                auto control = wnd->get_focused();
                if (control)
                {
                    wnd->send_event_to_control(control, ev);
                }
                wnd->send_event_to_plains(ev);
            }
        break;
        case WM_USER:
            reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))->send_internal(internal_event_type::user_emitted, static_cast<int32_t>(w_param), static_cast<int32_t>(l_param));
        break;
        case WM_DEVICECHANGE:
            reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))->send_system(system_event_type::device_change, static_cast<int32_t>(w_param), static_cast<int32_t>(l_param));
        break;
        case WM_DESTROY:
        {
            window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            wnd->graphic_.release();

            auto transient_window_ = wnd->get_transient_window();
            if (transient_window_)
            {
                transient_window_->enable();
            }

            if (wnd->close_callback)
            {
                wnd->close_callback();
            }

            wnd->context_.hwnd = 0;
        }
        break;
        default:
            return DefWindowProc(hwnd, message, w_param, l_param);
    }
    return 0;
}
}
