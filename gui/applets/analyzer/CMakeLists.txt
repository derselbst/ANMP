project(context-analyzer)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(analyzer_SRCS
    AnalyzerApplet.cpp
    AnalyzerBase.cpp
    BlockAnalyzer.cpp
    ASCIIAnalyzer.cpp
    SpectrogramAnalyzer.cpp
    PaletteHandler.cpp
    fht.cpp
)

qt5_wrap_ui(UI_ANAL_HEADER
    AnalyzerApplet.ui
)

ADD_LIBRARY(${LIBANAL} STATIC ${UI_ANAL_HEADER} ${analyzer_SRCS})
