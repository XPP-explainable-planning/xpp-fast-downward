# - Find the SPOT LP solver.
# This code defines the following variables:
#
#  SPOT_FOUND                 - TRUE if SPOT was found.
#  SPOT_INCLUDE_DIRS          - Full paths to all include dirs.
#  SPOT_LIBRARIES             - Full paths to all libraries.
#  SPOT_RUNTIME_LIBRARY       - Full path to the dll file on windows
#
# Usage:
#  find_package(cplex)
#
# The location of SPOT can be specified using the environment variable
# or cmake parameter DOWNWARD_SPOT_ROOT. If different installations
# for 32-/64-bit versions and release/debug versions of SPOT are available,
# they can be specified with
#   DOWNWARD_SPOT_ROOT32
#   DOWNWARD_SPOT_ROOT64
#   DOWNWARD_SPOT_ROOT_RELEASE32
#   DOWNWARD_SPOT_ROOT_RELEASE64
#   DOWNWARD_SPOT_ROOT_DEBUG32
#   DOWNWARD_SPOT_ROOT_DEBUG64
# More specific paths are preferred over less specific ones when searching
# for libraries.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

foreach(BITWIDTH 32 64)
    foreach(BUILDMODE "RELEASE" "DEBUG")
		set(SPOT_HINT_PATHS_${BUILDMODE}${BITWIDTH}
            ${DOWNWARD_SPOT_ROOT_${BUILDMODE}${BITWIDTH}}
            $ENV{DOWNWARD_SPOT_ROOT_${BUILDMODE}${BITWIDTH}}
            ${DOWNWARD_SPOT_ROOT${BITWIDTH}}
            $ENV{DOWNWARD_SPOT_ROOT${BITWIDTH}}
            ${DOWNWARD_SPOT_ROOT}
			/mnt/data_server/eifler/LTL2BA/install/
        )
    endforeach()
endforeach()

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(SPOT_HINT_PATHS_RELEASE ${SPOT_HINT_PATHS_RELEASE32})
    set(SPOT_HINT_PATHS_DEBUG ${SPOT_HINT_PATHS_DEBUG32})
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(SPOT_HINT_PATHS_RELEASE ${SPOT_HINT_PATHS_RELEASE64})
    set(SPOT_HINT_PATHS_DEBUG ${SPOT_HINT_PATHS_DEBUG64})
else()
    message(WARNING "Bitwidth could not be detected, preferring 32-bit version of SPOT")
    set(SPOT_HINT_PATHS_RELEASE
        ${SPOT_HINT_PATHS_RELEASE32}
        ${SPOT_HINT_PATHS_RELEASE64}
    )
    set(SPOT_HINT_PATHS_DEBUG
        ${SPOT_HINT_PATHS_DEBUG32}
        ${SPOT_HINT_PATHS_DEBUG64}
    )
endif()


if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(SPOT_LIBRARY_PATH_SUFFIX_RELEASE ${SPOT_LIBRARY_PATH_SUFFIX_RELEASE_32})
    set(SPOT_LIBRARY_PATH_SUFFIX_DEBUG ${SPOT_LIBRARY_PATH_SUFFIX_DEBUG_32})
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(SPOT_LIBRARY_PATH_SUFFIX_RELEASE ${SPOT_LIBRARY_PATH_SUFFIX_RELEASE_64})
    set(SPOT_LIBRARY_PATH_SUFFIX_DEBUG ${SPOT_LIBRARY_PATH_SUFFIX_DEBUG_64})
else()
    message(WARNING "Bitwidth could not be detected, preferring 32bit version of SPOT")
    set(SPOT_LIBRARY_PATH_SUFFIX_RELEASE
        ${SPOT_LIBRARY_PATH_SUFFIX_RELEASE_32}
        ${SPOT_LIBRARY_PATH_SUFFIX_RELEASE_64}
    )
    set(SPOT_LIBRARY_PATH_SUFFIX_DEBUG
        ${SPOT_LIBRARY_PATH_SUFFIX_DEBUG_32}
        ${SPOT_LIBRARY_PATH_SUFFIX_DEBUG_64}
    )
endif()


