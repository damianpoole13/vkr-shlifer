#include <wui/system/clipboard_tools.hpp>

#include <utf8/utf8.h>

#include <boost/nowide/convert.hpp>

#include <windows.h>


namespace wui
{

void clipboard_put(std::string_view text, system_context &context)
{
    auto wide_str = boost::nowide::widen(text);

    if (OpenClipboard(context.hwnd))
    {
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, wide_str.size() * sizeof(wchar_t) + 2);
        if (hGlobal != NULL)
        {
            LPVOID lpText = GlobalLock(hGlobal);
            memcpy(lpText, wide_str.c_str(), wide_str.size() * sizeof(wchar_t));

            EmptyClipboard();
            GlobalUnlock(hGlobal);

            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        CloseClipboard();
    }
}

bool is_text_in_clipboard(system_context &context)
{
    return IsClipboardFormatAvailable(CF_UNICODETEXT);
}

std::string clipboard_get_text(system_context &context)
{
    if (!OpenClipboard(NULL))
    {
        return "";
    }

    std::string paste_string;

    HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
    if (hglb)
    {
        wchar_t *lptstr = (wchar_t *)GlobalLock(hglb);
        if (lptstr)
        {
            paste_string = boost::nowide::narrow(lptstr);
            GlobalUnlock(hglb);
        }
    }
    CloseClipboard();

    return paste_string;
}

}
