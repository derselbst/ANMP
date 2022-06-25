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
        configdialog.h
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.slots.cpp
        mainwindow.callback.cpp
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
    
    qt_generate_dbus_interface(
        mainwindow.h # from that header
        org.anmp.xml # generate the dbus interface to that xml
        OPTIONS -M # exporting public slots only
    )

    # from the just generated xml file, generate implementation files for the dbus adaptor
    qt_add_dbus_adaptor(GUI_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.anmp.xml mainwindow.h MainWindow mainwindow_adaptor MainWindowAdaptor)
    
    # skip processing those auto generated files
    set_property(SOURCE mainwindow_adaptor.cpp mainwindow_adaptor.h PROPERTY SKIP_AUTOGEN ON)

    
    qt_wrap_ui(UI_HEADERS
        mainwindow.ui
        configdialog.ui
        songinspector.ui
        playcontrol.ui
    )

    # auto-generate "build/gui/anmp_dbus_interface.h" and "build/gui/anmp_dbus_interface.cpp" but skip
    # AUTOMOC and AUTOUIC for them (Policy CMP0071)
    qt_add_dbus_interface(ANMP_LAUNCHER_SRC ${CMAKE_CURRENT_BINARY_DIR}/org.anmp.xml anmp_dbus_interface)
    set_property(SOURCE ${ANMP_LAUNCHER_SRC} PROPERTY SKIP_AUTOGEN ON)    
    
    add_executable(${PROJECT_NAME} ${UI_HEADERS} ${GUI_SRCS} ${ANMP_LAUNCHER_SRC} icons.qrc)
    
    # this will add all necessary compile and link flags
    target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets Qt6::DBus Qt6::Svg)

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
        anmp
        ${LD_FLAGS}
    )
    
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
    
#     if(DEFINED DBUS_SERVICES_INSTALL_DIR)
#         install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.anmp.xml DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
#     else(DEFINED DBUS_SERVICES_INSTALL_DIR)
#         message(WARNING "DBUS_SERVICES_INSTALL_DIR is not defined, refusing to install anmp's dbus interface")
#     endif(DEFINED DBUS_SERVICES_INSTALL_DIR)
endif(USE_GUI)
