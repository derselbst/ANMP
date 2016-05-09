project(anmp-core)

include_directories(${PROJECT_INCLUDE_DIR})


# we should actually do this in the AudioOutput subdir, but since we need to handle anmp-normalize somewhere, search for ebur128 at this point
if(ENABLE_EBUR128)
  MY_FIND_LIB(EBUR128 ebur128)
endif(ENABLE_EBUR128)

### these subdirs make up the anmp-core library
add_subdirectory(AudioOutput)
add_subdirectory(Common)
add_subdirectory(InputLibraryWrapper)
add_subdirectory(PlayerLogic)
add_subdirectory(GUI)


if(EBUR128_FOUND)
    add_executable(anmp-normalize anmp-normalize.cpp)
    target_link_libraries(anmp-normalize ${LIB_ANMP_CORE})
endif(EBUR128_FOUND)
