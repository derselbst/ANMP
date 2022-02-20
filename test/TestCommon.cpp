
#include <iostream>
#include <string>

#include "Common.h"
#include "Test.h"

using namespace std;

int main()
{
    bool equal = iEquals("qwer", "qwer");
    TEST_ASSERT_EQ(equal, true);

    equal = iEquals("poiuasdfyxcvqwer", "poiuasdfyxcvqwer");
    TEST_ASSERT_EQ(equal, true);

    equal = iEquals("1234", "1234");
    TEST_ASSERT_EQ(equal, true);

    equal = iEquals("asdfasdf ASDFaSdF", "aSDFAsdf aSdFasDf");
    TEST_ASSERT_EQ(equal, true);

    equal = iEquals("XYZ123", "xyz123");
    TEST_ASSERT_EQ(equal, true);

    equal = iEquals("", "");
    TEST_ASSERT_EQ(equal, true);

    equal = iEquals("aaaa", "aaa");
    TEST_ASSERT_EQ(equal, false);

    equal = iEquals("zzz", "ZZZz");
    TEST_ASSERT_EQ(equal, false);

    string s = ::myHomeDir();
    TEST_ASSERT(s.size() >= 1);
    
    string test = "/var/tmp/song.midi";
    s = mybasename(test);
    TEST_ASSERT_EQ(s, "song.midi");
    s = mydirname(test);
    TEST_ASSERT_EQ(s, "/var/tmp");
    
    test = "/song2.suffix.mp3";
    s = mybasename(test);
    TEST_ASSERT_EQ(s, "song2.suffix.mp3" );
    s = mydirname(test);
    TEST_ASSERT_EQ(s, "/");
    
    test = "song2.suffix.mp3";
    s = mybasename(test);
    TEST_ASSERT_EQ(s, "song2.suffix.mp3");
    s = mydirname(test);
    TEST_ASSERT_EQ(s, ".");
    
    test = "../../bin/../song3.wav";
    s = mybasename(test);
    TEST_ASSERT_EQ(s, "song3.wav");
    s = mydirname(test);
    TEST_ASSERT_EQ(s, "../../bin/..");
    
    test = "/home/user/";
    s = mybasename(test);
    TEST_ASSERT_EQ(s, "user");
    s = mydirname(test);
    TEST_ASSERT_EQ(s, "/home");

    test = "/home/user";
    s = mybasename(test);
    TEST_ASSERT_EQ(s, "user");
    s = mydirname(test);
    TEST_ASSERT_EQ(s, "/home");

    test = ".";
    s = mybasename(test);
    TEST_ASSERT_EQ(s, test);
    s = mydirname(test);
    TEST_ASSERT_EQ(s, test);

    return 0;
}
