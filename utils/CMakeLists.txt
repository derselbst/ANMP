
if(EBUR128_FOUND)
    ### name for anmp's normalization tool
    SET(ANMP_NORM anmp-normalize)

    add_executable(${ANMP_NORM} anmp-normalize.cpp)
    set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
    
    target_link_libraries(${ANMP_NORM} anmp-core)
    if (CMAKE_COMPILER_IS_GNUCXX)
      target_link_libraries(${ANMP_NORM} stdc++fs)
    endif (CMAKE_COMPILER_IS_GNUCXX)
    
    install(TARGETS ${ANMP_NORM} DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    
    set_property(TARGET ${ANMP_NORM} PROPERTY CXX_STANDARD 14)
    set_property(TARGET ${ANMP_NORM} PROPERTY CXX_STANDARD_REQUIRED ON)
endif(EBUR128_FOUND) 
