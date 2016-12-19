project(anmp-player)

SET(ANMP_PLAYER_SRC    
       Config.cpp
       Player.cpp
       Playlist.cpp
)

#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_PLAYER_SRC})
