
#pragma once

#include <iostream>


#define TEST_ASSERT(COND) \
if(!(COND)) \
{ \
    std::cerr << __FILE__ << ":" << __LINE__ << " assertion (" << #COND << ") failed" << std::endl; \
    std::exit(-1); \
}
