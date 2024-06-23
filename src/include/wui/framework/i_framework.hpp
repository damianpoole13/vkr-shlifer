

#pragma once

#include <wui/common/error.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace wui
{

class i_framework
{
public:
    virtual void run() = 0;
    virtual void stop() = 0;

    virtual bool runned() const = 0;

    virtual error get_error() const = 0;

    virtual ~i_framework() {}
};

}
