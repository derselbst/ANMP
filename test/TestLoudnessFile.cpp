
#include <iostream>
#include <string>

#include "LoudnessFile.h"
#include "Test.h"

using namespace std;


int main()
{
    const string testFile = "test.ebur128";

    float gain = 2.0f;
    LoudnessFile::write(testFile, gain);
    TEST_ASSERT(LoudnessFile::read(testFile) == gain);

    gain = 0.0001f;
    LoudnessFile::write(testFile, gain);
    TEST_ASSERT(LoudnessFile::read(testFile) == gain);

    gain = 0.00f;
    LoudnessFile::write(testFile, gain);
    TEST_ASSERT(LoudnessFile::read(testFile) == gain);

    gain = -0.00f;
    LoudnessFile::write(testFile, gain);
    TEST_ASSERT(LoudnessFile::read(testFile) == gain);

    gain = -1.9876f;
    LoudnessFile::write(testFile, gain);
    TEST_ASSERT(LoudnessFile::read(testFile) == gain);

    return 0;
}
