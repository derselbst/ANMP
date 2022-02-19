project(anmp)


### these subdirs make up the anmp-core library
add_subdirectory(AudioOutput)
add_subdirectory(Common)
add_subdirectory(InputLibraryWrapper)
add_subdirectory(PlayerLogic)


add_library(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC anmp-audio-output anmp-common anmp-input anmp-player)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LD_FLAGS})
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})


install(FILES Common/PlaylistFactory.h Common/types.h Common/Event.h Common/Nullable.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(FILES InputLibraryWrapper/SongInfo.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(FILES PlayerLogic/IPlaylist.h PlayerLogic/Playlist.h PlayerLogic/Player.h PlayerLogic/Config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

