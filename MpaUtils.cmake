set_property(GLOBAL PROPERTY USE_FOLDERS ON)

cmake_policy(SET CMP0155 OLD)

option(BUILD_MPA_UTILS "build mpa utils" ON)
option(WITH_RPATH_LIBRARY "with rpath library for linux" ${LINUX})

if(WITH_RPATH_LIBRARY AND NOT DEFINED RPATH_LIBRARY_INSTALL_DIR)
    set(RPATH_LIBRARY_INSTALL_DIR bin)
endif()

set(Boost_NO_WARN_NEW_VERSIONS ON)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON) see below
set(MPAUTILS_DIR ${CMAKE_CURRENT_LIST_DIR})
set(MAAUTILS_DIR ${MPAUTILS_DIR})

list(APPEND CMAKE_MODULE_PATH "${MPAUTILS_DIR}/cmake/modules")

set(MPADEPS_DIR "" CACHE PATH "Path to the project-owned MpaDeps dependency root")

set(_MPADEPS_CANDIDATE "")
if(DEFINED MPADEPS_DIR AND NOT "${MPADEPS_DIR}" STREQUAL "" AND NOT MPADEPS_DIR MATCHES "-NOTFOUND$")
    get_filename_component(_MPADEPS_CANDIDATE "${MPADEPS_DIR}" ABSOLUTE)
    if(EXISTS "${_MPADEPS_CANDIDATE}/mpadeps.cmake")
        set(MPADEPS_DIR "${_MPADEPS_CANDIDATE}")
    else()
        message(WARNING
            "Ignoring stale MPADEPS_DIR cache entry: ${_MPADEPS_CANDIDATE}. "
            "Falling back to repository-local detection."
        )
        set(MPADEPS_DIR "")
    endif()
elseif(DEFINED ENV{MPADEPS_DIR} AND NOT "$ENV{MPADEPS_DIR}" STREQUAL "")
    get_filename_component(_MPADEPS_CANDIDATE "$ENV{MPADEPS_DIR}" ABSOLUTE)
    if(EXISTS "${_MPADEPS_CANDIDATE}/mpadeps.cmake")
        set(MPADEPS_DIR "${_MPADEPS_CANDIDATE}")
    else()
        message(WARNING
            "Ignoring stale ENV{MPADEPS_DIR}: ${_MPADEPS_CANDIDATE}. "
            "Falling back to repository-local detection."
        )
        set(MPADEPS_DIR "")
    endif()
endif()

if(NOT MPADEPS_DIR AND EXISTS "${MPAUTILS_DIR}/MpaDeps/mpadeps.cmake")
    get_filename_component(MPADEPS_DIR "${MPAUTILS_DIR}/MpaDeps" ABSOLUTE)
elseif(NOT MPADEPS_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/MpaDeps/mpadeps.cmake")
    get_filename_component(MPADEPS_DIR "${CMAKE_SOURCE_DIR}/MpaDeps" ABSOLUTE)
endif()

unset(_MPADEPS_CANDIDATE)

if(NOT MPADEPS_DIR OR NOT EXISTS "${MPADEPS_DIR}/mpadeps.cmake")
    message(FATAL_ERROR
        "Project-owned MpaDeps was not found.\n"
        "Expected ${MPAUTILS_DIR}/MpaDeps, ${CMAKE_SOURCE_DIR}/MpaDeps, or set MPADEPS_DIR explicitly.\n"
        "Bootstrap it with the local tools/mpadeps-download.py helper first."
    )
endif()

set(MAADEPS_DIR "${MPADEPS_DIR}")
include("${MPADEPS_DIR}/mpadeps.cmake")

# Basic compile and link configuration
include(${MPAUTILS_DIR}/cmake/config.cmake)
include(${MPAUTILS_DIR}/cmake/utils.cmake)
include(${MPAUTILS_DIR}/cmake/version.cmake)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Boost REQUIRED CONFIG COMPONENTS system regex)
find_package(ZLIB REQUIRED CONFIG)
find_package(fastdeploy_ppocr REQUIRED)
find_package(ONNXRuntime REQUIRED)

find_program(CCACHE_PROG ccache)

if(CCACHE_PROG)
    message("Find ccache at ${CCACHE_PROG}")

    if(ENABLE_CCACHE)
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROG})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROG})
    endif()
endif()

include_directories(${MPAUTILS_DIR}/include)

if(BUILD_MPA_UTILS AND NOT TARGET MpaUtils)
    add_subdirectory(${MPAUTILS_DIR}/source ${CMAKE_CURRENT_BINARY_DIR}/MpaUtils)
endif()
