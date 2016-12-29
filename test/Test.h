
#pragma once

#include <iostream>


#define TEST_ASSERT(COND) \
if(!(COND)) \
{ \
    std::cerr << "assertion (" << #COND << ") failed" << std::endl; \
    return -1; \
}
