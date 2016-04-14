project(context-analyzer)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# qt5_use_modules(app Core Gui Widgets OpenGL)


  # Tell CMake to run moc when necessary:
  set(CMAKE_AUTOMOC ON)
  # As moc files are generated in the binary dir, tell CMake
  # to always look for includes there:
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  # Qt library
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5OpenGL REQUIRED)
#   find_package(QtOpenGL REQUIRED)
  
  # OpenGL & GLEW library
  find_package(OpenGL REQUIRED)
  
  set(CMAKE_AUTOMOC ON)

  # The Qt5Widgets_INCLUDES also includes the include directories for
  # dependencies QtCore and QtGui
  include_directories(${Qt5Widgets_INCLUDES} ${OPENGL_INCLUDE_DIR} ${Qt5OpenGL_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE} /usr/include/qt5/QtOpenGL/ /usr/include/qt5 /usr/include/qt5/QtCore/ /usr/include/qt5/QtGui/ /usr/include/qt5/QtWidgets/ ../../../Common ../../../PlayerLogic ../../../InputLibraryWrapper ../../../AudioOutput)

  # We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
  add_definitions(${Qt5Widgets_DEFINITIONS})

  # Executables fail to build with Qt 5 in the default configuration
  # without -fPIE. We add that here.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

set(analyzer_SRCS
#     AnalyzerApplet.cpp
    AnalyzerBase.cpp
#     BallsAnalyzer.cpp
    BlockAnalyzer.cpp
#     DiscoAnalyzer.cpp
# #     ASCIIAnalyzer.cpp
    PaletteHandler.cpp
    fht.cpp
)


# kde4_add_plugin(amarok_context_applet_analyzer ${analyzer_SRCS})
# 
# if(APPLE)
#     SET_TARGET_PROPERTIES(amarok_context_applet_analyzer PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
# endif(APPLE)


# target_link_libraries(amarok_context_applet_analyzer
#     amarokcore
#     amaroklib
#     ${KDE4_PLASMA_LIBS}
#     ${QT_QTOPENGL_LIBRARY}
#     ${OPENGL_gl_LIBRARY}
# )

SET(LIBANAL analyzer)
ADD_LIBRARY(${LIBANAL} ${analyzer_SRCS})