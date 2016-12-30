
project(anmp-test)


ADD_EXECUTABLE(TestLoudnessFile TestLoudnessFile.cpp)
TARGET_LINK_LIBRARIES(TestLoudnessFile anmp)
ADD_TEST(NAME TestLoudnessFile COMMAND TestLoudnessFile)

ADD_EXECUTABLE(TestCommon TestCommon.cpp)
TARGET_LINK_LIBRARIES(TestCommon anmp)
ADD_TEST(NAME TestCommon COMMAND TestCommon)

ADD_EXECUTABLE(TestStandardWrapper TestStandardWrapper.cpp)
TARGET_LINK_LIBRARIES(TestStandardWrapper anmp)
ADD_TEST(NAME TestStandardWrapper COMMAND TestStandardWrapper)
