
project(anmp-test)

ENABLE_TESTING()
include ( AddAnmpTest )

# first define the test target, used by the macros below
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --output-on-failure)

ADD_ANMP_TEST(TestLoudnessFile)
ADD_ANMP_TEST(TestCommon)
ADD_ANMP_TEST(TestConfigSerialization)
ADD_ANMP_TEST(TestStandardWrapper)
