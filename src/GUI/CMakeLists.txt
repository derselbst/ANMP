project(anmp-gui)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt library
find_package(Qt5Widgets REQUIRED)
#   find_package(Qt5Declarative REQUIRED)
#   find_package(Qt5OpenGL REQUIRED)
#   find_package(QtOpenGL REQUIRED)

# OpenGL & GLEW library
find_package(OpenGL REQUIRED)

# The Qt5Widgets_INCLUDES also includes the include directories for
# dependencies QtCore and QtGui
include_directories(${Qt5Widgets_INCLUDES} ${OPENGL_INCLUDE_DIR} ${Qt5OpenGL_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE} /usr/include/qt5/QtOpenGL/ /usr/include/qt5 /usr/include/qt5/QtCore/ /usr/include/qt5/QtGui/ /usr/include/qt5/QtWidgets/ ../../../Common ../../../PlayerLogic ../../../InputLibraryWrapper ../../../AudioOutput)

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
#     ${Qt5Core_QTMAIN_LIBRARIES}
    ${LD_FLAGS}
    analyzer
)