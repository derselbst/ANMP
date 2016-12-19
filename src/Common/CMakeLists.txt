project(anmp-common)

SET(ANMP_COMMON_SRC
       AtomicWrite.cpp
       Common.cpp
       LoudnessFile.cpp
       PlaylistFactory.cpp
       SongFormat.cpp
)


#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_COMMON_SRC})
