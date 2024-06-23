

#pragma once

#include <wui/locale/locale_type.hpp>
#include <wui/common/error.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace wui
{

class i_locale
{
public:
    virtual locale_type get_type() const = 0;
    virtual std::string get_name() const = 0;

    virtual void set(std::string_view section, std::string_view value, std::string_view str) = 0;
    virtual const std::string &get(std::string_view section, std::string_view value) const = 0;


    virtual void load_resource(int32_t resource_index, std::string_view resource_section) = 0;
    virtual void load_json(std::string_view json) = 0;
    virtual void load_file(std::string_view file_name) = 0;
    virtual void load_locale(const i_locale &locale_) = 0;

    virtual error get_error() const = 0;

    virtual ~i_locale() {}
};

}
