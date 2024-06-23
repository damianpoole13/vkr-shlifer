
#include <wui/system/path_tools.hpp>

namespace wui
{

std::string real_path(std::string_view relative_path)
{
    return relative_path.data();
}

}
