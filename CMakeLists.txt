cmake_minimum_required(VERSION 2.6)
project (ANMP)

 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
 
# set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
 
# include_directories("${PROJECT_INCLUDE_DIR}")
# include_directories("${PROJECT_SOURCE_DIR}")

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)


add_subdirectory(src)