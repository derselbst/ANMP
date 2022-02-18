
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(EBUR128_FOUND)
    ### name for anmp's normalization tool
    SET(ANMP_NORM anmp-normalize)

    add_executable(${ANMP_NORM} anmp-normalize.cpp)
    set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
    
    target_link_libraries(${ANMP_NORM} anmp)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if(USE_LIBCXX)
            target_link_libraries(${ANMP_NORM} libc++experimental.a)
        else(USE_LIBCXX)
            target_link_libraries(${ANMP_NORM} stdc++fs)
        endif(USE_LIBCXX)
    endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    
    install(TARGETS ${ANMP_NORM} DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    
    set_property(TARGET ${ANMP_NORM} PROPERTY CXX_STANDARD 17)
    set_property(TARGET ${ANMP_NORM} PROPERTY CXX_STANDARD_REQUIRED ON)
endif(EBUR128_FOUND)


SET(ANMP_DUMP anmp-dump)

add_executable(${ANMP_DUMP} anmp-dump.cpp)
set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")

target_link_libraries(${ANMP_DUMP} anmp)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if(USE_LIBCXX)
            target_link_libraries(${ANMP_DUMP} libc++experimental.a)
        else(USE_LIBCXX)
            target_link_libraries(${ANMP_DUMP} stdc++fs)
        endif(USE_LIBCXX)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

install(TARGETS ${ANMP_DUMP} DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)

set_property(TARGET ${ANMP_DUMP} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${ANMP_DUMP} PROPERTY CXX_STANDARD_REQUIRED ON)
