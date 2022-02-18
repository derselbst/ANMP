project(anmp-player)

SET(ANMP_PLAYER_SRC    
       Config.cpp
       Config.h
       IPlaylist.h
       Player.cpp
       Player.h
       Playlist.cpp
       Playlist.h
)

#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_PLAYER_SRC})
