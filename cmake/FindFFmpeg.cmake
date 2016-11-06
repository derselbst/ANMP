# FindFFMPEG
# ----------
#
# Find the native FFMPEG includes and libraries
#
# This module defines:
#
#  FFMPEG_INCLUDE_DIRS, where to find avformat.h, avcodec.h...
#  FFMPEG_LIBRARIES, the libraries to link against to use FFMPEG.
#  FFMPEG_FOUND, If false, do not try to use FFMPEG.
#
# also defined, but not for general use are:
#
#   FFMPEG_avformat_LIBRARY, where to find the FFMPEG avformat library.
#   FFMPEG_avcodec_LIBRARY, where to find the FFMPEG avcodec library.
#
# This is useful to do it this way so that we can always add more libraries
# if needed to ``FFMPEG_LIBRARIES`` if ffmpeg ever changes...

#=============================================================================
# Copyright: 1993-2008 Ken Martin, Will Schroeder, Bill Lorensen
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of ppsspp, substitute the full
#  License text for the above reference.)

find_path(FFMPEG_INCLUDE_DIR1 libavformat/avformat.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
#   $ENV{FFMPEG_DIR}/libavformat
#   $ENV{FFMPEG_DIR}/include/libavformat
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
#   /usr/include/libavformat
#   /usr/include/ffmpeg/libavformat
#   /usr/local/include/libavformat
)

find_path(FFMPEG_INCLUDE_DIR2 libavcodec/avcodec.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
#   $ENV{FFMPEG_DIR}/libavcodec
#   $ENV{FFMPEG_DIR}/include/libavcodec
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
#   /usr/include/libavcodec
#   /usr/include/ffmpeg/libavcodec
#   /usr/local/include/libavcodec
)

find_path(FFMPEG_INCLUDE_DIR3 libavutil/avutil.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
#   $ENV{FFMPEG_DIR}/libavutil
#   $ENV{FFMPEG_DIR}/include/libavutil
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
#   /usr/include/libavutil
#   /usr/include/ffmpeg/libavutil
#   /usr/local/include/libavutil
)

find_path(FFMPEG_INCLUDE_DIR4 libswresample/swresample.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
#   $ENV{FFMPEG_DIR}/libswresample
#   $ENV{FFMPEG_DIR}/include/libswresample
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
#   /usr/include/libswresample
#   /usr/include/ffmpeg/libswresample
#   /usr/local/include/libswresample
)

if(FFMPEG_INCLUDE_DIR1 AND
   FFMPEG_INCLUDE_DIR2 AND
   FFMPEG_INCLUDE_DIR3 AND
   FFMPEG_INCLUDE_DIR4
)
  set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR1}
                         ${FFMPEG_INCLUDE_DIR2}
                         ${FFMPEG_INCLUDE_DIR3}
                         ${FFMPEG_INCLUDE_DIR4}
  )
endif()

find_library(FFMPEG_avformat_LIBRARY avformat
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib${CMAKE_LIBDIR_SUFFIX}
  $ENV{FFMPEG_DIR}/libavformat
  /usr/local/lib${CMAKE_LIBDIR_SUFFIX}
  /usr/lib${CMAKE_LIBDIR_SUFFIX}
)

find_library(FFMPEG_avcodec_LIBRARY avcodec
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib${CMAKE_LIBDIR_SUFFIX}
  $ENV{FFMPEG_DIR}/libavcodec
  /usr/local/lib${CMAKE_LIBDIR_SUFFIX}
  /usr/lib${CMAKE_LIBDIR_SUFFIX}
)

find_library(FFMPEG_avutil_LIBRARY avutil
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib${CMAKE_LIBDIR_SUFFIX}
  $ENV{FFMPEG_DIR}/libavutil
  /usr/local/lib${CMAKE_LIBDIR_SUFFIX}
  /usr/lib${CMAKE_LIBDIR_SUFFIX}
)

find_library(FFMPEG_swresample_LIBRARY swresample
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib${CMAKE_LIBDIR_SUFFIX}
  $ENV{FFMPEG_DIR}/libswresample
  /usr/local/lib${CMAKE_LIBDIR_SUFFIX}
  /usr/lib${CMAKE_LIBDIR_SUFFIX}
)

if(  FFMPEG_INCLUDE_DIRS AND
     FFMPEG_avformat_LIBRARY AND
     FFMPEG_avcodec_LIBRARY AND
     FFMPEG_avutil_LIBRARY AND
     FFMPEG_swresample_LIBRARY
  )
    set(FFMPEG_FOUND "TRUE")
    set(FFMPEG_LIBRARIES ${FFMPEG_avformat_LIBRARY}
                         ${FFMPEG_avcodec_LIBRARY}
                         ${FFMPEG_avutil_LIBRARY}
                         ${FFMPEG_swresample_LIBRARY}
    )
endif()

