project(anmp-gui)

include_directories(${PROJECT_INCLUDE_DIR})


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt library
MY_FIND_PKG(Qt5Widgets Qt5Widgets)

# OpenGL & GLEW library
MY_FIND_PKG(OPENGL OpenGL)

message(STATUS ${OPENGL_FOUND})
message(STATUS ${Qt5Widgets_FOUND})

if(Qt5Widgets_FOUND AND OPENGL_FOUND)
    # The Qt5Widgets_INCLUDES also includes the include directories for
    # dependencies QtCore and QtGui
#     include_directories(${Qt5Widgets_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})

    # We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
    add_definitions(${Qt5Widgets_DEFINITIONS})

    # Executables fail to build with Qt 5 in the default configuration
    # without -fPIE. We add that here.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

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

    add_subdirectory(applets)


    add_executable(ANMP ${UI_HEADERS} ${GUI_SRCS} icons.qrc)
    qt5_use_modules(ANMP Core Gui Widgets OpenGL)

    target_link_libraries(ANMP
        ${OPENGL_LIBRARY}
        ${LD_FLAGS}
        analyzer
        ${LIB_ANMP_CORE}
    )
else(Qt5Widgets_FOUND AND OPENGL_FOUND)
    message(WARNING "QT GUI will not be build")
endif(Qt5Widgets_FOUND AND OPENGL_FOUND)
