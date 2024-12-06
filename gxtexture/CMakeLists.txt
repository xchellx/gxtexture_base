# gxtexture cmake list

cmake_minimum_required(VERSION 3.28, FATAL_ERROR)

project(gxtexture LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../extern/cmake-modules")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_TAG --tags)
git_local_changes(GIT_LOCAL_CHANGES)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/gxtexture_version.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/gxtexture_version.h.in" "${PROJECT_SOURCE_DIR}/include/configure/gxtexture_version.h" @ONLY)
endif()

option(GX_INCLUDE_DECODE "Include decoding capabilities within gxtexture." ON)
option(GX_INCLUDE_ENCODE "Include encoding capabilities within gxtexture." ON)
option(GX_COMP_RGBA "Use RGBA colors instead of BGRA colors." OFF)
option(GX_COMP_ARGB "Use ARGB colors instead of BGRA colors." OFF)
option(GX_COMP_ABGR "Use ABGR colors instead of BGRA colors." OFF)

if(GX_COMP_RGBA)
    set(GX_COMP_SH_B "16")
    set(GX_COMP_SH_G "8")
    set(GX_COMP_SH_R "0")
    set(GX_COMP_SH_A "24")
elseif(GX_COMP_ARGB)
    set(GX_COMP_SH_B "24")
    set(GX_COMP_SH_G "16")
    set(GX_COMP_SH_R "8")
    set(GX_COMP_SH_A "0")
elseif(GX_COMP_ABGR)
    set(GX_COMP_SH_B "8")
    set(GX_COMP_SH_G "16")
    set(GX_COMP_SH_R "24")
    set(GX_COMP_SH_A "0")
else()
    set(GX_COMP_BGRA ON)
    set(GX_COMP_SH_B "0")
    set(GX_COMP_SH_G "8")
    set(GX_COMP_SH_R "16")
    set(GX_COMP_SH_A "24")
endif()

option(GXTEXTURE_STATIC "Build static library." ON)
if(GXTEXTURE_STATIC)
    set(GXTEXTURE_BUILD_TYPE STATIC)
    set(GXTEXTURE_LINK_TYPE ARCHIVE)
    set(GXTEXTURE_IS_SHARED OFF)
else()
    set(GXTEXTURE_BUILD_TYPE SHARED)
    set(GXTEXTURE_LINK_TYPE LIBRARY)
    set(GXTEXTURE_IS_SHARED ON)
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/gxtexture_settings.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/gxtexture_settings.h.in" "${PROJECT_SOURCE_DIR}/include/configure/gxtexture_settings.h" @ONLY)
endif()

add_library(gxtexture ${GXTEXTURE_BUILD_TYPE}
    ${PROJECT_SOURCE_DIR}/include/gxtexture.h
    
    ${PROJECT_SOURCE_DIR}/src/gxtexture.c)

target_include_directories(gxtexture
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../extern/stb)

target_compile_features(gxtexture
    PRIVATE
        c_std_99
        c_function_prototypes
        c_variadic_macros)

set_target_properties(gxtexture
    PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON)

target_precompile_headers(gxtexture
    PUBLIC
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/gxtexture_version.h>"
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/gxtexture_settings.h>")

# stdext
if (NOT TARGET stdext)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../extern/stdext CMAKE/extern/stdext)
endif()
target_link_libraries(gxtexture PUBLIC stdext)

# squish
if(NOT TARGET squish)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../extern/libsquish CMAKE/extern/libsquish)
endif()
target_link_libraries(gxtexture PUBLIC squish)

# octree_color_quantizer
if(NOT TARGET octree_color_quantizer)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../octree_color_quantizer CMAKE/octree_color_quantizer)
endif()
target_link_libraries(gxtexture PUBLIC octree_color_quantizer)

install(TARGETS gxtexture
    ${GXTEXTURE_LINK_TYPE} DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
