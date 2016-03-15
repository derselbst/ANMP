cmake_minimum_required(VERSION 2.6)
project (ANMP)

 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
 
# set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
 
# include_directories("${PROJECT_INCLUDE_DIR}")
# include_directories("${PROJECT_SOURCE_DIR}")

add_subdirectory(src)