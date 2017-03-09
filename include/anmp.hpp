
#pragma once


#include "types.h"
#include "Config.h"
#include "Common.h"
#include "AtomicWrite.h"
#include "StringFormatter.h"

#include "Playlist.h"
#include "PlaylistFactory.h"
#include "Player.h"

#include "Song.h"

#include "IAudioOutput.h"


#define ANMP_TITLE "ANMP"
#define ANMP_SUBTITLE "Another Nameless Music Player"
#define ANMP_VERSION "3"
#define ANMP_API_VERSION 0
#define ANMP_WEBSITE "https://www.github.com/derselbst/ANMP"
#define ANMP_COPYRIGHT "Tom Moebert (derselbst)"

constexpr const char* GetCompilerUsed()
{
    #if defined(__clang__)
        return "Clang";

    #elif defined(__ICC) || defined(__INTEL_COMPILER)
        return "Intel ICC/ICPC";

    #elif defined(__GNUC__) || defined(__GNUG__)
        return "GNU GCC/G++";

    #elif defined(__HP_cc) || defined(__HP_aCC)
        return "Hewlett-Packard C/aC++"

    #elif defined(__IBMC__) || defined(__IBMCPP__)
        return "IBM XL C/C++";

    #elif defined(_MSC_VER)
        return "Microsoft Visual Studio";

    #elif defined(__PGI)
        return "Portland Group PGCC/PGCPP";

    #elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
        return "Oracle Solaris Studio";
        
    #else
        return "unknown";

    #endif
}

constexpr const char* GetCompilerVersionUsed()
{
    #if defined(__clang__)
        return __clang_version__;

    #elif defined(__ICC) || defined(__INTEL_COMPILER)
        return __VERSION__;

    #elif defined(__GNUC__) || defined(__GNUG__)
        return __VERSION__;

    #elif defined(__HP_cc) || defined(__HP_aCC)
        return __HP_aCC;

    #elif defined(__IBMC__) || defined(__IBMCPP__)
        return __xlc__;

    #elif defined(_MSC_VER)
        return _MSC_FULL_VER;

    #elif defined(__PGI)
        return __PGIC__ "." __PGIC_MINOR "." __PGIC_PATCHLEVEL__ ;

    #elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
        return __SUNPRO_CC;
        
    #else
        return "";

    #endif
}