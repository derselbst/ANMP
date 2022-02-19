project(anmp-common)

SET(ANMP_COMMON_SRC
       AtomicWrite.cpp
       AtomicWrite.h
       AudioDriver.c
       AudioDriver.h
       Common.cpp
       Common.h
       CommonExceptions.h
       Event.h
       LoudnessFile.cpp
       LoudnessFile.h
       Nullable.h
       PlaylistFactory.cpp
       PlaylistFactory.h
       SongFormat.cpp
       SongFormat.h
       StringFormatter.cpp
       StringFormatter.h
       SampleFormat.c
       SampleFormat.h
       tree.h
       types.h
)

add_library(${PROJECT_NAME} OBJECT ${ANMP_COMMON_SRC})

if(USE_LIBCUE)
  target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::LIBCUE)
endif()
