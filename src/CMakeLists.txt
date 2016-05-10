project(anmp-core)

include_directories(${PROJECT_INCLUDE_DIR})

### setup library names we will build in the subdirs
SET(LIB_ANMP_CORE ${PROJECT_NAME})


### these subdirs make up the anmp-core library
add_subdirectory(AudioOutput)
add_subdirectory(Common)
add_subdirectory(InputLibraryWrapper)
add_subdirectory(PlayerLogic)
# add_subdirectory(GUI)


# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-export-all-symbols")
add_library(${LIB_ANMP_CORE} SHARED $<TARGET_OBJECTS:anmp-audio-output>
                                    $<TARGET_OBJECTS:anmp-common>
                                    $<TARGET_OBJECTS:anmp-input>
                                    $<TARGET_OBJECTS:anmp-player>)
target_link_libraries(${LIB_ANMP_CORE} ${LD_FLAGS})


if(EBUR128_FOUND)
    add_executable(anmp-normalize anmp-normalize.cpp)
    set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
    target_link_libraries(anmp-normalize ${LIB_ANMP_CORE})
endif(EBUR128_FOUND)
