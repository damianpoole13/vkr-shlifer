
#include <wui/system/uri_tools.hpp>


#include <windows.h>

#include <boost/nowide/convert.hpp>

namespace wui
{

bool open_uri(std::string_view uri)
{
    return reinterpret_cast<int64_t>(ShellExecute(NULL, L"open", boost::nowide::widen(uri).c_str(), NULL, NULL, SW_SHOW)) < 32;     
}

}
