file(GLOB_RECURSE mpa_utils_src *.h *.hpp *.cpp)
file(GLOB_RECURSE mpa_utils_header
    ${MPAUTILS_DIR}/include/MpaUtils/*
    ${MPAUTILS_DIR}/include/meojson/*
)

add_library(MpaUtils SHARED ${mpa_utils_src} ${mpa_utils_header})
target_include_directories(MpaUtils
    PUBLIC
        $<BUILD_INTERFACE:${MPAUTILS_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(MpaUtils PRIVATE Boost::regex Boost::system ${OpenCV_LIBS})

if(WIN32)
    target_link_libraries(MpaUtils PRIVATE d3d12 dxgi Cfgmgr32)
endif()

if(LINUX)
    target_link_libraries(MpaUtils PRIVATE pthread dl)
endif()

target_compile_definitions(MpaUtils PRIVATE MAA_UTILS_EXPORTS)
set_target_properties(MpaUtils PROPERTIES OUTPUT_NAME MpaUtils)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${mpa_utils_src})
