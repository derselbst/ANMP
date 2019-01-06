macro ( ADD_ANMP_TEST _test )
    ADD_EXECUTABLE(${_test} ${_test}.cpp )
    
    # only build this unit test when explicitly requested by "make check"
    set_target_properties(${_test} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    
    TARGET_LINK_LIBRARIES(${_test} anmp)
    
    # add the test to ctest
    ADD_TEST(NAME ${_test} COMMAND ${_test})
    
    # append the current unit test to check-target as dependency
    add_dependencies(check ${_test})

endmacro ( ADD_ANMP_TEST )
