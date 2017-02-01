
if(EBUR128_FOUND)
    ### name for anmp's normalization tool
    SET(ANMP_NORM anmp-normalize)

    add_executable(${ANMP_NORM} anmp-normalize.cpp)
    set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
    
    target_link_libraries(${ANMP_NORM} anmp)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      target_link_libraries(${ANMP_NORM} stdc++fs)
    endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    
    install(TARGETS ${ANMP_NORM} DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    
    set_property(TARGET ${ANMP_NORM} PROPERTY CXX_STANDARD 14)
    set_property(TARGET ${ANMP_NORM} PROPERTY CXX_STANDARD_REQUIRED ON)
endif(EBUR128_FOUND) 



SET(ANMP_LOOP anmp-loopify)

add_executable(${ANMP_LOOP} anmp-loopify.cpp)
set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")

target_link_libraries(${ANMP_LOOP} anmp)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(${ANMP_LOOP} stdc++fs)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

install(TARGETS ${ANMP_LOOP} DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)

set_property(TARGET ${ANMP_LOOP} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${ANMP_LOOP} PROPERTY CXX_STANDARD_REQUIRED ON)