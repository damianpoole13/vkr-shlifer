
#pragma once

#include <wui/framework/i_framework.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace wui
{

namespace framework
{

class framework_win_impl : public i_framework
{
public:
    framework_win_impl();

    virtual void run();
    virtual void stop();

    virtual bool runned() const;

    virtual error get_error() const;

private:
    bool runned_;

    error err;
};

}


}

