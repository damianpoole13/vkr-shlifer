#pragma once

#include <wui/window/window.hpp>
#include <wui/control/button.hpp>

#include <MainSheet/MainSheet.h>
#include <ButtonSheet/ButtonSheet.h>
#include <InputSheet/InputSheet.h>

enum class Sheet
{
    Main,
    Window,
    Button,
    Input,
    List,
    Menu,
    Others
};

class MainFrame
{
public:
    MainFrame();

    void Run();

private:
    static const int32_t WND_WIDTH = 800, WND_HEIGHT = 600;

    std::shared_ptr<wui::window> window;

    std::shared_ptr<wui::button> mainSheet, windowSheet, buttonSheet, inputSheet, listSheet, menuSheet, panelSheet;
    std::shared_ptr<wui::button> accountButton, menuButton;

    Sheet sheet;

    MainSheet mainSheetImpl;
    ButtonSheet buttonSheetImpl;
    InputSheet inputSheetImpl;

    void ReceiveEvents(const wui::event &ev);

    void UpdateSheetsSize();
    void UpdateSheets();
};
