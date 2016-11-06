# This file is copyrighted under the BSD-license for buildsystem files of KDE
# copyright 2010, Patrick von Reth <patrick.vonreth@gmail.com>
#
#
# - Try to find the libsamplerate library
# Once done this will define
#
#  LIBSAMPLERATE_FOUND          Set to TRUE if libsamplerate librarys and include directory is found
#  LIBSAMPLERATE_LIBRARIES      The libsamplerate librarys
#  LIBSAMPLERATE_INCLUDE_DIRS   The libsamplerate include directory
#  LIBSAMPLERATE_DEFINITIONS    cflags and so

if (LIBSAMPLERATE_LIBRARIES AND LIBSAMPLERATE_INCLUDE_DIRS)
  # in cache already
  set(LIBSAMPLERATE_FOUND TRUE)
  
else (LIBSAMPLERATE_LIBRARIES AND LIBSAMPLERATE_INCLUDE_DIRS)
	find_package(PkgConfig)
	    if (PKG_CONFIG_FOUND)
		pkg_check_modules(PC_LIBSAMPLERATE samplerate>=0.1.9)# due to constness of SRC_DATA::data_in
		
		set(LIBSAMPLERATE_LIBRARIES ${PC_LIBSAMPLERATE_LIBRARIES})
	    endif (PKG_CONFIG_FOUND)

	set(LIBSAMPLERATE_DEFINITIONS ${PC_LIBSAMPLERATE_CFLAGS_OTHER})

# 	find_library(LIBSAMPLERATE_LIBRARIES NAMES samplerate libsamplerate
# 					HINTS ${PC_LIBSAMPLERATE_LIBDIR} ${PC_LIBSAMPLERATE_LIBRARY_DIRS})

	find_path(LIBSAMPLERATE_INCLUDE_DIRS samplerate.h
				HINTS ${PC_LIBSAMPLERATE_INCLUDEDIR} ${PC_LIBSAMPLERATE_INCLUDE_DIRS})

	if(LIBSAMPLERATE_LIBRARIES AND LIBSAMPLERATE_INCLUDE_DIRS)
	    set(LIBSAMPLERATE_FOUND TRUE)
# 	    message(STATUS "Found libsamplerate ${LIBSAMPLERATE_LIBRARIES}")
	else(LIBSAMPLERATE_LIBRARIES AND LIBSAMPLERATE_PLUGIN_PATH)
# 	    message(STATUS "Could not find libsamplerate, get it http://www.mega-nerd.com/SRC/")
	endif(LIBSAMPLERATE_LIBRARIES AND LIBSAMPLERATE_INCLUDE_DIRS)

endif(LIBSAMPLERATE_LIBRARIES AND LIBSAMPLERATE_INCLUDE_DIRS)
