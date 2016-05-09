project(anmp-common)

include_directories(${PROJECT_INCLUDE_DIR})

SET(ANMP_COMMON_SRC
       AtomicWrite.cpp
       Common.cpp
       LoudnessFile.cpp
       PlaylistFactory.cpp
)


option(ENABLE_CUE "Use libcue to support loading cue sheets" ON)

if(ENABLE_CUE)
  PKG_CHECK_MODULES(LIBCUE libcue)
  
  if(LIBCUE_FOUND)
        add_definitions(-DUSE_CUE)
        INCLUDE_DIRECTORIES(${LIBCUE_INCLUDE_DIRS})
#         link_directories(${LIBCUE_LIBRARY_DIRS})
        set(LD_FLAGS ${LD_FLAGS} ${LIBCUE_LIBRARIES})
        
        MY_PRINT(LIBCUE)
  endif(LIBCUE_FOUND)
endif(ENABLE_CUE)

#---------------------------------------------------------------------------------------------

ADD_LIBRARY(${LIB_ANMP_COMMON} ${ANMP_COMMON_SRC})
target_link_libraries(${LIB_ANMP_COMMON} ${LD_FLAGS})
