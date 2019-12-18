# - Find the SPOT LP solver.
# This code defines the following variables:
#
#  SPOT_FOUND                 - TRUE if SPOT was found.
#  SPOT_INCLUDE_DIRS          - Full paths to all include dirs.
#  SPOT_LIBRARIES             - Full paths to all libraries.
#
# Usage:
#  find_package(clp)
#
# Hints to the location of SPOT can be specified using the variables
# SPOT_HINT_PATHS_RELEASE and SPOT_HINT_PATHS_DEBUG. This is used by
# FindOsi to locate the SPOT version that is shipped with the OSI version
# which was found.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

foreach(BUILDMODE "RELEASE" "DEBUG")
    set(_CMAKE_FIND_ROOT_PATH_${BUILDMODE}
            ${CMAKE_FIND_ROOT_PATH}
            $ENV{SPOT_HINT_PATHS})
    find_path(SPOT_INCLUDE_DIRS
        NAMES bddx.h
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES include
    )
    find_library(SPOT_BDDX_LIBRARY_${BUILDMODE}
        NAMES bddx
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES lib
    )
    find_library(SPOT_SPOT_LIBRARY_${BUILDMODE}
        NAMES spot
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES lib
    )
    find_library(SPOT_SPOTGEN_LIBRARY_${BUILDMODE}
        NAMES spotgen
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES lib
    )
    find_library(SPOT_SPOTLTSMIN_LIBRARY_${BUILDMODE}
        NAMES spotltsmin
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES lib
    )
endforeach()

set(SPOT_LIBRARIES
    optimized ${SPOT_SPOT_LIBRARY_RELEASE}
    optimized ${SPOT_BDDX_LIBRARY_RELEASE}
    optimized ${SPOT_SPOTGEN_LIBRARY_RELEASE}
    optimized ${SPOT_SPOTLTSMIN_LIBRARY_RELEASE}
    debug ${SPOT_SPOT_LIBRARY_DEBUG}
    debug ${SPOT_BDDX_LIBRARY_DEBUG}
    debug ${SPOT_SPOTGEN_LIBRARY_DEBUG}
    debug ${SPOT_SPOTLTSMIN_LIBRARY_DEBUG})

# Check if everything was found and set SPOT_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    spot
    REQUIRED_VARS SPOT_INCLUDE_DIRS SPOT_LIBRARIES
)

mark_as_advanced(SPOT_LIBRARY_RELEASE SPOT_LIBRARY_DEBUG SPOT_INCLUDE_DIRS SPOT_LIBRARIES)
