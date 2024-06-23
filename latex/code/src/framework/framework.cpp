#include <wui/framework/framework.hpp>

#include <wui/framework/framework_win_impl.hpp>

#include <wui/framework/i_framework.hpp>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#endif

#include <memory>
#include <iostream>

namespace wui
{

namespace framework
{

static std::shared_ptr<i_framework> instance = nullptr;

/// Interface

void init()
{
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void run()
{
    if (instance)
    {
        return;
    }
    instance = std::make_shared<framework_win_impl>();

    instance->run();
}

void stop()
{
    if (instance)
    {
        instance->stop();
    }
    instance.reset();
}

bool runned()
{
    return instance != nullptr;
}

error get_error()
{
    if (instance)
    {
        return instance->get_error();
    }
    return {};
}

}

}
