project(anmp-core)

include_directories(${PROJECT_INCLUDE_DIR})

### setup library names we will build in the subdirs
SET(LIB_ANMP_CORE anmp)


### these subdirs make up the anmp-core library
add_subdirectory(AudioOutput)
add_subdirectory(Common)
add_subdirectory(InputLibraryWrapper)
add_subdirectory(PlayerLogic)


# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-export-all-symbols")
add_library(${LIB_ANMP_CORE} $<TARGET_OBJECTS:anmp-audio-output>
                                    $<TARGET_OBJECTS:anmp-common>
                                    $<TARGET_OBJECTS:anmp-input>
                                    $<TARGET_OBJECTS:anmp-player>)
target_link_libraries(${LIB_ANMP_CORE} ${LD_FLAGS})
install(TARGETS ${LIB_ANMP_CORE} DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(EBUR128_FOUND)
    add_executable(anmp-normalize anmp-normalize.cpp)
    set(LINK_FLAGS ${LINK_FLAGS} "-Wl,-whole-archive")
    target_link_libraries(anmp-normalize ${LIB_ANMP_CORE})
    install(TARGETS anmp-normalize DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif(EBUR128_FOUND)


install(FILES Common/PlaylistFactory.h Common/types.h Common/Event.h Common/Nullable.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_ANMP_CORE})
install(FILES InputLibraryWrapper/SongInfo.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_ANMP_CORE})
install(FILES PlayerLogic/IPlaylist.h PlayerLogic/Playlist.h PlayerLogic/Player.h PlayerLogic/Config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_ANMP_CORE})


add_subdirectory(GUI)
