
#pragma once

#include <iostream>


#define TEST_ASSERT(COND) \
if(!(COND)) \
{ \
    std::cerr << __FILE__ << ":" << __LINE__ << " assertion (" << #COND << ") failed" << std::endl; \
    std::exit(-1); \
}

#define TEST_ASSERT_EQ(LHS, RHS) \
if((LHS) != (RHS)) \
{ \
     throw AssertionException(__FILE__ ":"  #__LINE__ " : assertion failed: " #LHS "==" #RHS);
}
