
#pragma once

#include <iostream>


#define TEST_ASSERT(COND) \
if(!(COND)) \
{ \
    std::cerr << __FILE__ << ":" << __LINE__ << " assertion (" << #COND << ") failed" << std::endl; \
    return -1; \
}
