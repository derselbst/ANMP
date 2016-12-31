
#include <iostream>
#include <string>

#include "Common.h"
#include "Test.h"

using namespace std;

int main()
{
    bool equal = iEquals("qwer", "qwer");
    TEST_ASSERT(equal == true);
    
    equal = iEquals("poiuasdfyxcvqwer", "poiuasdfyxcvqwer");
    TEST_ASSERT(equal == true);
    
    equal = iEquals("1234", "1234");
    TEST_ASSERT(equal == true);
    
    equal = iEquals("asdfasdf ASDFaSdF", "aSDFAsdf aSdFasDf");
    TEST_ASSERT(equal == true);
    
    equal = iEquals("XYZ123", "xyz123");
    TEST_ASSERT(equal == true);
    
    equal = iEquals("", "");
    TEST_ASSERT(equal == true);
    
    equal = iEquals("aaaa", "aaa");
    TEST_ASSERT(equal == false);
    
    equal = iEquals("zzz", "ZZZz");
    TEST_ASSERT(equal == false);
    
    return 0;
}
