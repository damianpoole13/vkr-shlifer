#include <wui/common/error.hpp>
#include <map>

namespace wui
{

std::string str(error_type t)
{
    static const std::map<error_type, std::string> names = {
        { error_type::ok,             "ok"             },
        { error_type::file_not_found, "file_not_found" },
        { error_type::invalid_json,   "invalid_json"   },
        { error_type::invalid_value,  "invalid_value"  },
        { error_type::system_error,   "system_error"   },
        { error_type::no_handle,      "no_handle"      },
        { error_type::already_runned, "already_runned" }
    };

    auto n = names.find(t);
    if (n != names.end())
    {
        return n->second;
    }

    return "";
}

}
