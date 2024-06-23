#ifdef _WIN32

#include <wui/framework/framework_win_impl.hpp>

#include <windows.h>

namespace wui
{

namespace framework
{

framework_win_impl::framework_win_impl()
    : runned_(false),
    err{}
{
}

void framework_win_impl::run()
{
    if (runned_)
    {
        err.type = error_type::already_runned;
        err.component = "framework_win_impl::run()";

        return;
    }
    runned_ = true;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void framework_win_impl::stop()
{
    if (runned_)
    {
        PostQuitMessage(IDCANCEL);
        runned_ = false;

        err.reset();
    }
}

bool framework_win_impl::runned() const
{
    return runned_;
}

error framework_win_impl::get_error() const
{
    return err;
}

}

}

#endif
