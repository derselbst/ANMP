project(anmp-gui)

include_directories(${PROJECT_INCLUDE_DIR})


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)


if(QT5_FOUND)

    # name of libraries that will be build in the applets subdir
    SET(LIBANAL analyzer)
    add_subdirectory(applets)


    set(GUI_SRCS
        configdialog.cpp
        main.cpp
        mainwindow.cpp
        overlaycontrol.cpp
        PlaylistModel.cpp
        PlaylistView.cpp
    )

    qt5_wrap_ui(UI_HEADERS
        mainwindow.ui
        configdialog.ui
        overlaycontrol.ui
    )

    add_executable(anmp ${UI_HEADERS} ${GUI_SRCS} icons.qrc)
    qt5_use_modules(anmp Core Gui Widgets OpenGL)

    target_link_libraries(anmp
        ${LIBANAL}
        ${LIB_ANMP_CORE}
        ${LD_FLAGS}
    )
    install(TARGETS anmp DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(QT5_FOUND)
