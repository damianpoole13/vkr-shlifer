#pragma once

#include <wui/window/window.hpp>

#include <wui/control/input.hpp>
#include <wui/control/text.hpp>

class InputSheet
{
public:
    InputSheet();

    void Run(std::weak_ptr<wui::window> parentWindow_);
    void End();

    void UpdateSize(int32_t width, int32_t height);

private:
    std::weak_ptr<wui::window> parentWindow_;
    
    std::shared_ptr<wui::text> inputText;
    std::shared_ptr<wui::input> input0;
};
