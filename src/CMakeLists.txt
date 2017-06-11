project(anmp)



### these subdirs make up the anmp-core library
add_subdirectory(AudioOutput)
add_subdirectory(Common)
add_subdirectory(InputLibraryWrapper)
add_subdirectory(PlayerLogic)



add_library(${PROJECT_NAME} $<TARGET_OBJECTS:anmp-audio-output>
                            $<TARGET_OBJECTS:anmp-common>
                            $<TARGET_OBJECTS:anmp-input>
                            $<TARGET_OBJECTS:anmp-player>)
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 0)
target_link_libraries(${PROJECT_NAME} ${LD_FLAGS})
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

# this target requires c++11
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)



install(FILES Common/PlaylistFactory.h Common/types.h Common/Event.h Common/Nullable.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(FILES InputLibraryWrapper/SongInfo.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(FILES PlayerLogic/IPlaylist.h PlayerLogic/Playlist.h PlayerLogic/Player.h PlayerLogic/Config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

