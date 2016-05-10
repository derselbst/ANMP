project(anmp-common)

include_directories(${PROJECT_INCLUDE_DIR})

SET(ANMP_COMMON_SRC
       AtomicWrite.cpp
       Common.cpp
       LoudnessFile.cpp
       PlaylistFactory.cpp
)


#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_COMMON_SRC})
