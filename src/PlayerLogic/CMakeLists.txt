project(anmp-player)

include_directories(${PROJECT_INCLUDE_DIR})

SET(ANMP_PLAYER_SRC    
       Config.cpp
       Player.cpp
       Playlist.cpp
)

#---------------------------------------------------------------------------------------------

ADD_LIBRARY(${LIB_ANMP_PLAYER} ${ANMP_PLAYER_SRC})
target_link_libraries(${LIB_ANMP_PLAYER} ${LIB_ANMP_INPUT}) # because of SongFormat::operator!=

