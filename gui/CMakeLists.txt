cmake_minimum_required(VERSION 3.0 FATAL_ERROR) # due to CMAKE_AUTOUIC and CMAKE_AUTORCC

project(anmp-qt)



# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)


if(USE_GUI)

    set(GUI_SRCS
        configdialog.cpp
        main.cpp
        mainwindow.cpp
        mainwindow.slots.cpp
        mainwindow.callback.cpp
        overlaycontrol.cpp
        PlaylistModel.cpp
        PlaylistView.cpp
        PlayheadSlider.cpp
#         ${CMAKE_CURRENT_BINARY_DIR}/org.anmp.xml # cause this xml to be a dependency and thus force it to get generated
    )
    
    qt5_generate_dbus_interface(
        mainwindow.h # from that header
        org.anmp.xml # generate the dbus interface to that xml
        OPTIONS -M # exporting public slots only
    )

    qt5_add_dbus_adaptor(GUI_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.anmp.xml mainwindow.h MainWindow mainwindowadaptor MainWindowAdaptor)

    qt5_wrap_ui(UI_HEADERS
        mainwindow.ui
        configdialog.ui
        overlaycontrol.ui
    )

    add_executable(${PROJECT_NAME} ${UI_HEADERS} ${GUI_SRCS} icons.qrc)
    
    # should add all necessary compile and link flags
    qt5_use_modules(${PROJECT_NAME} Core OpenGL)

    if(USE_VISUALIZER)
        # name of libraries that will be build in the applets subdir
        SET(LIBANAL analyzer)
        add_subdirectory(applets)
        target_link_libraries(${PROJECT_NAME} ${LIBANAL})
    endif(USE_VISUALIZER)
    
    
    target_link_libraries(${PROJECT_NAME}
        anmp
        ${LD_FLAGS}
    )
    
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
#     install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.anmp.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
endif(USE_GUI)
