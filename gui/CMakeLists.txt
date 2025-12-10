
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
        configdialog.h
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.slots.cpp
        mainwindow.callback.cpp
        mpris2.cpp
        mpris2.h
        PlayheadSlider.cpp
        PlayheadSlider.h
        PlaylistModel.cpp
        PlaylistModel.h
        PlaylistView.cpp
        PlaylistView.h
        ChannelConfigModel.cpp
        ChannelConfigModel.h
        ChannelConfigView.cpp
        ChannelConfigView.h
        songinspector.cpp
        songinspector.h
    )
    set(ANMP_LAUNCHER_SRC "")
    
    qt_wrap_ui(UI_HEADERS
        mainwindow.ui
        configdialog.ui
        songinspector.ui
        playcontrol.ui
    )

    add_executable(${PROJECT_NAME} ${UI_HEADERS} ${GUI_SRCS} ${ANMP_LAUNCHER_SRC} icons.qrc)

    # this will add all necessary compile and link flags
    target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets Qt6::Svg $<$<BOOL:${USE_DBUS}>:Qt6::DBus>)

    if(USE_VISUALIZER)
        # name of libraries that will be build in the applets subdir
        SET(LIBANAL analyzer)
        add_subdirectory(applets/analyzer)
        target_link_libraries(${PROJECT_NAME} ${LIBANAL})
    endif(USE_VISUALIZER)

    if(WIN32)
      set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    target_link_libraries(${PROJECT_NAME}
        anmp-internal
        ${LD_FLAGS}
    )
    
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
    
#     if(DEFINED DBUS_SERVICES_INSTALL_DIR)
#         install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.anmp.xml DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
#     else(DEFINED DBUS_SERVICES_INSTALL_DIR)
#         message(WARNING "DBUS_SERVICES_INSTALL_DIR is not defined, refusing to install anmp's dbus interface")
#     endif(DEFINED DBUS_SERVICES_INSTALL_DIR)
endif(USE_GUI)
