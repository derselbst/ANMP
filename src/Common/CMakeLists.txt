project(anmp-common)

SET(ANMP_COMMON_SRC
       AtomicWrite.cpp
       AudioDriver.c
       Common.cpp
       LoudnessFile.cpp
       PlaylistFactory.cpp
       SongFormat.cpp
       StringFormatter.cpp
       SampleFormat.c
)


#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_COMMON_SRC})
