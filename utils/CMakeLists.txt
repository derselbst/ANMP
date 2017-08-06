
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
    
    set_property(TARGET ${ANMP_NORM} PROPERTY CXX_STANDARD 14)
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

set_property(TARGET ${ANMP_DUMP} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${ANMP_DUMP} PROPERTY CXX_STANDARD_REQUIRED ON)


SET(ANMP_LOOP anmp-loopify)

add_executable(${ANMP_LOOP} anmp-loopify.cpp)
set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")

target_link_libraries(${ANMP_LOOP} anmp)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if(USE_LIBCXX)
            target_link_libraries(${ANMP_LOOP} libc++experimental.a)
        else(USE_LIBCXX)
            target_link_libraries(${ANMP_LOOP} stdc++fs)
        endif(USE_LIBCXX)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

install(TARGETS ${ANMP_LOOP} DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)

set_property(TARGET ${ANMP_LOOP} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${ANMP_LOOP} PROPERTY CXX_STANDARD_REQUIRED ON)


if(USE_GUI)

    SET(ANMP_LAUNCHER anmp-launcher)
    SET(ANMP_LAUNCHER_SRC anmp-launcher.cpp)
#     PKGCONFIG_GETVAR(dbus-1 prefix DBUS_PREFIX)
#     set(network_xml ${DBUS_PREFIX}/interfaces/org.foo.bar.xml)

    qt5_add_dbus_interface(ANMP_LAUNCHER_SRC ${CMAKE_CURRENT_BINARY_DIR}/../gui/org.anmp.xml anmp_dbus_interface)
    
    add_executable(${ANMP_LAUNCHER} ${ANMP_LAUNCHER_SRC})
    set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")

    target_link_libraries(${ANMP_LAUNCHER} ${Qt5DBus_LIBRARIES} ${Qt5Widgets_LIBRARIES})

    install(TARGETS ${ANMP_LAUNCHER} DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)

    set_property(TARGET ${ANMP_LAUNCHER} PROPERTY CXX_STANDARD 11)
    set_property(TARGET ${ANMP_LAUNCHER} PROPERTY CXX_STANDARD_REQUIRED ON)

endif(USE_GUI)
