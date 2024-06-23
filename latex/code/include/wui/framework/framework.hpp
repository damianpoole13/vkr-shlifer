

#pragma once

#include <wui/common/error.hpp>

namespace wui
{

namespace framework
{

void init();

void run();
void stop();

bool runned();

error get_error();

}

}
