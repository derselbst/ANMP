macro(to_yes_no vars)
  foreach(var ${ARGV})
    if(${var})
      set(${var} "yes")
    else()
      set(${var} "no ")
    endif()
  endforeach()
endmacro()

macro(if_empty_print_missing vars)
  foreach(var ${ARGV})
    if(NOT ${var})
      set(${var} "<not set>")
    endif()
  endforeach()
endmacro()


macro(find_pkg_config prefix pkgname)
    find_package(PkgConfig ${ARGV2})
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(${prefix}_PKGCONF ${ARGV2} ${pkgname})
        if(${${prefix}_PKGCONF_FOUND})
            message(STATUS "${pkgname} library dirs: ${${prefix}_PKGCONF_LIBRARY_DIRS}")
            message(STATUS "${pkgname} cflags: ${${prefix}_PKGCONF_CFLAGS_OTHER}")
            message(STATUS "${pkgname} include dirs: ${${prefix}_PKGCONF_INCLUDE_DIRS}")
            message(STATUS "${pkgname} libraries: ${${prefix}_PKGCONF_LIBRARIES}")
            message(STATUS "${pkgname} ldflags: ${${prefix}_PKGCONF_LDFLAGS_OTHER}")

            set(${prefix}_FOUND ${${prefix}_PKGCONF_FOUND})
            set(${prefix}_CFLAGS ${${prefix}_PKGCONF_CFLAGS_OTHER})
            to_space_list(${prefix}_CFLAGS)
            set(${prefix}_INCLUDE_DIRS ${${prefix}_PKGCONF_INCLUDE_DIRS})
            foreach(lib ${${prefix}_PKGCONF_LIBRARIES})
                string(TOUPPER ${lib} LIB)
                find_library(${prefix}_${LIB}_LIBRARY ${lib}
                    HINTS ${${prefix}_PKGCONF_LIBRARY_DIRS})
                mark_as_advanced(${prefix}_${LIB}_LIBRARY)
                list(APPEND ${prefix}_LIBRARIES ${${prefix}_${LIB}_LIBRARY})
            endforeach()
            list(APPEND ${prefix}_LIBRARIES ${${prefix}_PKGCONF_LDFLAGS_OTHER})
        endif()
    endif()
endmacro()

macro(MY_PRINT prefix)
        message("\t${prefix} include dir  : ${${prefix}_INCLUDE_DIR} ${${prefix}_INCLUDE_DIRS}")
        message("\t${prefix} libraries    : ${${prefix}_LIBRARIES}")
        message("\t${prefix} static libs  : ${${prefix}_STATIC_LIBRARIES}")
endmacro()

macro(MY_FIND_LIB prefix libname)
    set (${prefix}_INSTALL_DIR "" CACHE FILEPATH "Directory where ${prefix} is installed")

    FIND_LIBRARY(${prefix}_LIBRARIES ${libname} ${${prefix}_INSTALL_DIR})

    if(${prefix}_LIBRARIES)
        set(${prefix}_FOUND TRUE)
    
        add_definitions(-DUSE_${prefix})
        set (${prefix}_INCLUDE_DIR ${${prefix}_INCLUDE_DIR} "/usr/include/${libname}")
        set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${${prefix}_INCLUDE_DIR} ${${prefix}_INCLUDE_DIRS})
                
        set(LD_FLAGS ${LD_FLAGS} ${${prefix}_LIBRARIES})
        
        MY_PRINT(${prefix})
    else(${prefix}_LIBRARIES)
        set(${prefix}_FOUND FALSE)
    endif(${prefix}_LIBRARIES)
endmacro()

macro(MY_FIND_PKG prefix pkgname)
    find_package(${pkgname})

    HANDLE_FOUND(prefix)
endmacro()


macro(HANDLE_FOUND prefix)
    if(${prefix}_FOUND)
    
        set(PROJECT_SYSTEM_INCLUDE_DIRS ${PROJECT_SYSTEM_INCLUDE_DIRS} ${${prefix}_INCLUDE_DIR} ${${prefix}_INCLUDE_DIRS})
                
        set(LD_FLAGS ${LD_FLAGS} ${${prefix}_LIBRARIES})
        
        MY_PRINT(${prefix})
    endif(${prefix}_FOUND)
endmacro()