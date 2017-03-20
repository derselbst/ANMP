
project(anmp-test)


ADD_EXECUTABLE(TestLoudnessFile TestLoudnessFile.cpp)
TARGET_LINK_LIBRARIES(TestLoudnessFile anmp)
ADD_TEST(NAME TestLoudnessFile COMMAND TestLoudnessFile)

ADD_EXECUTABLE(TestCommon TestCommon.cpp)
TARGET_LINK_LIBRARIES(TestCommon anmp)
ADD_TEST(NAME TestCommon COMMAND TestCommon)

ADD_EXECUTABLE(TestConfigSerialization TestConfigSerialization.cpp)
TARGET_LINK_LIBRARIES(TestConfigSerialization anmp)
ADD_TEST(NAME TestConfigSerialization COMMAND TestConfigSerialization)

ADD_EXECUTABLE(TestStandardWrapper TestStandardWrapper.cpp)
TARGET_LINK_LIBRARIES(TestStandardWrapper anmp)
ADD_TEST(NAME TestStandardWrapper COMMAND TestStandardWrapper)

if(USE_JACK)
    ADD_EXECUTABLE(TestJackOutput TestJackOutput.cpp)
    TARGET_LINK_LIBRARIES(TestJackOutput anmp)
    # ADD_TEST(NAME TestJackOutput COMMAND TestJackOutput)
endif(USE_JACK)


# make test only runs tests, but doesnt build them
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}  DEPENDS TestLoudnessFile TestCommon TestConfigSerialization TestStandardWrapper TestJackOutput)
