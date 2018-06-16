project(context-analyzer)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(analyzer_SRCS
    AnalyzerApplet.cpp
    AnalyzerBase.cpp
#     BallsAnalyzer.cpp
    BlockAnalyzer.cpp
#     DiscoAnalyzer.cpp
    ASCIIAnalyzer.cpp
    PaletteHandler.cpp
    fht.cpp
)

qt5_wrap_ui(UI_HEADERS
    AnalyzerApplet.ui
)

ADD_LIBRARY(${LIBANAL} STATIC ${UI_HEADERS} ${analyzer_SRCS})
