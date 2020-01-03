
#pragma once


#include "AtomicWrite.h"
#include "Common.h"
#include "Config.h"
#include "StringFormatter.h"
#include "types.h"

#include "Player.h"
#include "Playlist.h"
#include "PlaylistFactory.h"

#include "Song.h"

#include "IAudioOutput.h"


#define ANMP_TITLE "ANMP"
#define ANMP_SUBTITLE "Another Nameless Music Player"
#define ANMP_VERSION "9"
#define ANMP_API_VERSION 0
#define ANMP_WEBSITE "https://www.github.com/derselbst/ANMP"
#define ANMP_COPYRIGHT "Tom Moebert (derselbst)"

// sadly concatination of c-strings doesnt work with constexpr variables, have to use macros here
#if defined(__clang__)
#define ANMP_COMPILER_USED "Clang"
#define ANMP_COMPILER_VERSION_USED __clang_version__

#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define ANMP_COMPILER_USED "Intel ICC/ICPC"
#define ANMP_COMPILER_VERSION_USED __VERSION__

#elif defined(__GNUC__) || defined(__GNUG__)
#define ANMP_COMPILER_USED "GNU GCC/G++"
#define ANMP_COMPILER_VERSION_USED __VERSION__

#elif defined(__HP_cc) || defined(__HP_aCC)
#define ANMP_COMPILER_USED "Hewlett-Packard C/aC++"
#define ANMP_COMPILER_VERSION_USED __HP_aCC

#elif defined(__IBMC__) || defined(__IBMCPP__)
#define ANMP_COMPILER_USED "IBM XL C/C++"
#define ANMP_COMPILER_VERSION_USED __xlc__

#elif defined(_MSC_VER)
#define ANMP_COMPILER_USED "Microsoft Visual Studio"
#define ANMP_COMPILER_VERSION_USED _MSC_FULL_VER

#elif defined(__PGI)
#define ANMP_COMPILER_USED "Portland Group PGCC/PGCPP"
#define ANMP_COMPILER_VERSION_USED __PGIC__ "." __PGIC_MINOR "." __PGIC_PATCHLEVEL__

#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#define ANMP_COMPILER_USED "Oracle Solaris Studio"
#define ANMP_COMPILER_VERSION_USED __SUNPRO_CC

#else
#warning "Using unknown compiler"
#define ANMP_COMPILER_USED "unknown"
#define ANMP_COMPILER_VERSION_USED ""

#endif
