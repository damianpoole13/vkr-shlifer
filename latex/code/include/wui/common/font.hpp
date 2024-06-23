
#pragma once

#include <cstdint>
#include <string>

namespace wui
{

enum class decorations : uint32_t
{
    normal = 0,
    bold = (1 << 0),
    italic = (1 << 1),
    underline = (1 << 2),
    strike_out = (1 << 3)
};

struct font
{
    std::string name;
    int32_t size;
    decorations decorations_;
};

}
