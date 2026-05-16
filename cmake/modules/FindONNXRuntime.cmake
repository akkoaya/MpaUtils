set(_ONNXRUNTIME_HINTS "")
if(DEFINED MPADEPS_DIR AND NOT "${MPADEPS_DIR}" STREQUAL "")
    list(APPEND _ONNXRUNTIME_HINTS
        "${MPADEPS_DIR}/vcpkg/installed/${MPADEPS_TRIPLET}"
        "${MPADEPS_DIR}/vcpkg/installed/${MPADEPS_TRIPLET}/include/onnxruntime"
    )
endif()

find_path(
    ONNXRuntime_INCLUDE_DIR
    NAMES onnxruntime/onnxruntime_c_api.h onnxruntime_c_api.h
    HINTS ${_ONNXRUNTIME_HINTS}
    PATH_SUFFIXES include include/onnxruntime
    NO_CACHE
)

find_library(
    ONNXRuntime_LIBRARY_IMP
    NAMES onnxruntime
    HINTS ${_ONNXRUNTIME_HINTS}
    PATH_SUFFIXES lib debug/lib
    NO_CACHE
)

if(WIN32 AND ONNXRuntime_LIBRARY_IMP)
    get_filename_component(ONNXRuntime_PATH_LIB "${ONNXRuntime_LIBRARY_IMP}" DIRECTORY)
    find_file(
        ONNXRuntime_LIBRARY
        NAMES onnxruntime_mpa.dll onnxruntime_maa.dll onnxruntime.dll
        PATHS "${ONNXRuntime_PATH_LIB}/../bin"
        NO_CACHE
    )
    find_file(
        ONNXRuntime_LIBRARY_IMP_DEBUG
        NAMES onnxruntime.lib
        PATHS "${ONNXRuntime_PATH_LIB}/../debug/lib"
        NO_CACHE
    )
    find_file(
        ONNXRuntime_LIBRARY_DEBUG
        NAMES onnxruntime_mpa.dll onnxruntime_maa.dll onnxruntime.dll
        PATHS "${ONNXRuntime_PATH_LIB}/../debug/bin"
        NO_CACHE
    )
else()
    set(ONNXRuntime_LIBRARY "${ONNXRuntime_LIBRARY_IMP}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ONNXRuntime
    REQUIRED_VARS ONNXRuntime_LIBRARY_IMP ONNXRuntime_INCLUDE_DIR
)

if(ONNXRuntime_FOUND)
    set(ONNXRuntime_INCLUDE_DIRS "${ONNXRuntime_INCLUDE_DIR}")
    if(NOT TARGET ONNXRuntime::ONNXRuntime)
        add_library(ONNXRuntime::ONNXRuntime SHARED IMPORTED)
        set_property(TARGET ONNXRuntime::ONNXRuntime APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        if(WIN32)
            set_property(TARGET ONNXRuntime::ONNXRuntime APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(
                ONNXRuntime::ONNXRuntime
                PROPERTIES
                    IMPORTED_IMPLIB_RELEASE "${ONNXRuntime_LIBRARY_IMP}"
                    IMPORTED_IMPLIB_DEBUG "${ONNXRuntime_LIBRARY_IMP_DEBUG}"
                    IMPORTED_LOCATION_DEBUG "${ONNXRuntime_LIBRARY_DEBUG}"
            )
        endif()
        set_target_properties(
            ONNXRuntime::ONNXRuntime
            PROPERTIES
                IMPORTED_LOCATION_RELEASE "${ONNXRuntime_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${ONNXRuntime_INCLUDE_DIR}"
        )
    endif()
endif()

unset(_ONNXRUNTIME_HINTS)
