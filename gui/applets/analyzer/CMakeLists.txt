project(context-analyzer)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(analyzer_SRCS
    AnalyzerApplet.cpp
    AnalyzerApplet.h
    AnalyzerBase.cpp
    AnalyzerBase.h
    ASCIIAnalyzer.cpp
    ASCIIAnalyzer.h
    BlockAnalyzer.cpp
    BlockAnalyzer.h
    SpectrogramAnalyzer.cpp
    SpectrogramAnalyzer.h
    PaletteHandler.cpp
    PaletteHandler.h
    fht.cpp
    fht.h
)

qt5_wrap_ui(UI_ANAL_HEADER
    AnalyzerApplet.ui
)

ADD_LIBRARY(${LIBANAL} STATIC ${UI_ANAL_HEADER} ${analyzer_SRCS})
