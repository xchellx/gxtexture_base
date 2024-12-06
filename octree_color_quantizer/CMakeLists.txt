# octree_color_quantizer cmake list

cmake_minimum_required(VERSION 3.28, FATAL_ERROR)

project(octree_color_quantizer LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../extern/cmake-modules")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_TAG --tags)
git_local_changes(GIT_LOCAL_CHANGES)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_version.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_version.h.in" "${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_version.h" @ONLY)
endif()

set(OCQ_MAX_DEPTH "8" CACHE STRING "The maximum color depth the quantizer will operate to.")
option(OCQ_COMP_RGBA "Use RGBA colors instead of BGRA colors." OFF)
option(OCQ_COMP_ARGB "Use ARGB colors instead of BGRA colors." OFF)
option(OCQ_COMP_ABGR "Use ABGR colors instead of BGRA colors." OFF)
option(STB_DS_IMPLEMENTED "Define this if you already have implemented stb_ds.h via STB_DS_IMPLEMENTATION." OFF)

if(OCQ_COMP_RGBA)
    set(OCQ_COMP_SH_B "16")
    set(OCQ_COMP_SH_G "8")
    set(OCQ_COMP_SH_R "0")
    set(OCQ_COMP_SH_A "24")
elseif(OCQ_COMP_ARGB)
    set(OCQ_COMP_SH_B "24")
    set(OCQ_COMP_SH_G "16")
    set(OCQ_COMP_SH_R "8")
    set(OCQ_COMP_SH_A "0")
elseif(OCQ_COMP_ABGR)
    set(OCQ_COMP_SH_B "8")
    set(OCQ_COMP_SH_G "16")
    set(OCQ_COMP_SH_R "24")
    set(OCQ_COMP_SH_A "0")
else()
    set(OCQ_COMP_BGRA ON)
    set(OCQ_COMP_SH_B "0")
    set(OCQ_COMP_SH_G "8")
    set(OCQ_COMP_SH_R "16")
    set(OCQ_COMP_SH_A "24")
endif()

if(NOT STB_DS_IMPLEMENTED)
    set(STB_DS_IMPLEMENTED ON)
    set(STB_DS_FILE ${PROJECT_SOURCE_DIR}/../extern/stb_helpers/stb_ds_impl.c)
else()
    set(STB_DS_FILE "")
endif()

option(OCTREE_COLOR_QUANTIZER_STATIC "Build static library." ON)
if(OCTREE_COLOR_QUANTIZER_STATIC)
    set(OCTREE_COLOR_QUANTIZER_BUILD_TYPE STATIC)
    set(OCTREE_COLOR_QUANTIZER_LINK_TYPE ARCHIVE)
    set(OCTREE_COLOR_QUANTIZER_IS_SHARED OFF)
else()
    set(OCTREE_COLOR_QUANTIZER_BUILD_TYPE SHARED)
    set(OCTREE_COLOR_QUANTIZER_LINK_TYPE LIBRARY)
    set(OCTREE_COLOR_QUANTIZER_IS_SHARED ON)
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_settings.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_settings.h.in" "${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_settings.h" @ONLY)
endif()

add_library(octree_color_quantizer ${OCTREE_COLOR_QUANTIZER_BUILD_TYPE}
    ${PROJECT_SOURCE_DIR}/include/octree_color_quantizer.h
    
    ${PROJECT_SOURCE_DIR}/src/octree_color_quantizer.c
    
    ${STB_DS_FILE})

target_include_directories(octree_color_quantizer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../extern/stb)

target_compile_features(octree_color_quantizer
    PRIVATE
        c_std_99
        c_function_prototypes
        c_variadic_macros)

set_target_properties(octree_color_quantizer
    PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON)

target_precompile_headers(octree_color_quantizer
    PUBLIC
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_version.h>"
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/octree_color_quantizer_settings.h>")

# stdext
if (NOT TARGET stdext)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../extern/stdext CMAKE/extern/stdext)
endif()
target_link_libraries(octree_color_quantizer PUBLIC stdext)

install(TARGETS octree_color_quantizer
    ${OCTREE_COLOR_QUANTIZER_LINK_TYPE} DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
