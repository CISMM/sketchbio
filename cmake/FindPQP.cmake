# - Find PQP
# Find the PQP headers and libraries
#
# PQP_INCLUDE_DIRS - where to find PQP.h
# PQP_LIBRARIES    - List of libraries when using PQP
# PQP_FOUND        - True if PQP found.
#
# Adapted from Findquatlib.cmake by Ryan Pavlik
# Adapted by: Shawn Waldon
#

if (TARGET PQP)
    # Look for the header file
    find_path(PQP_INCLUDE_DIR NAMES PQP.h
                PATHS ${PQP_SOURCE_DIR})
    set(PQP_LIBRARY "PQP")
else()
    set(QUATLIB_ROOT_DIR
        "${QUATLIB_ROOT_DIR}"
        CACHE
        PATH
        "Root directory to search for PQP")

    # no idea what this part does
    if ("${CMAKE_SIZEOF_VOID_P}" MATCHES "8")
        set(_libsuffixes lib64 lib)

        # 64-bit dir: only set on win64
        file(TO_CMAKE_PATH "$ENV{ProgramW6432}" _progfiles)
    else()
        set(_libsuffixes lib)
        if (NOT "$ENV{ProgramFiles(x86)}" STREQUAL "")
            # 32-bit dir: only set on win64
            file(TO_CMAKE_PATH "$ENV{ProgramFiles(x86)}" _progfiles)
        else()
            # 32-bit dir on win32, useless to win64
            file(TO_CMAKE_PATH "$ENV{ProgramFiles}" _progfiles)
        endif()
    endif()

    # look for the header file
    find_path(PQP_INCLUDE_DIR
        NAMES
        PQP.h
        HINTS
        "${PQP_ROOT_DIR}"
        PATH_SUFFIXES
        include
        PATHS
        "${_progfiles}/PQP"
        C:/usr/local
        /usr/local)

    # Look for the library
    find_library(PQP_LIBRARY
        NAMES
        PQP.lib
        libPQP.a
        HINTS
        "${PQP_ROOT_DIR}"
        PATH_SUFFIXES
        "${_progfiles}/PQP"
        C:/usr/local
        /usr/local)
endif()

# handle the QUIETLY and REQUIRED arguments and set PQP_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PQP
    DEFAULT_MSG
    PQP_LIBRARY
    PQP_INCLUDE_DIR)

if (PQP_FOUND)
    set(PQP_LIBRARIES ${PQP_LIBRARY})
    set(PQP_INCLUDE_DIRS ${PQP_INCLUDE_DIR})

    mark_as_advanced(PQP_ROOT_DIR)
else()
    set(PQP_LIBRARIES)
    set(PQP_INCLUDE_DIRS)
endif()

mark_as_advanced(PQP_LIBRARY PQP_INCLUDE_DIR)
