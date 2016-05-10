project(anmp-player)

include_directories(${PROJECT_INCLUDE_DIR})

SET(ANMP_PLAYER_SRC    
       Config.cpp
       Player.cpp
       Playlist.cpp
)

#---------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} OBJECT ${ANMP_PLAYER_SRC})
