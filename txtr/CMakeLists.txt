# txtr cmake list

cmake_minimum_required(VERSION 3.28, FATAL_ERROR)

project(txtr LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../extern/cmake-modules")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_TAG --tags)
git_local_changes(GIT_LOCAL_CHANGES)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/txtr_version.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/txtr_version.h.in" "${PROJECT_SOURCE_DIR}/include/configure/txtr_version.h" @ONLY)
endif()

option(TXTR_INCLUDE_ERROR_STRINGS "Include extended error strings within txtr." OFF)
option(TXTR_INCLUDE_DECODE "Include decoding capabilities within txtr." ON)
option(TXTR_INCLUDE_ENCODE "Include encoding capabilities within txtr." ON)
option(TXTR_COMP_RGBA "Use RGBA colors instead of BGRA colors." OFF)
option(TXTR_COMP_ARGB "Use ARGB colors instead of BGRA colors." OFF)
option(TXTR_COMP_ABGR "Use ABGR colors instead of BGRA colors." OFF)
option(STB_IMAGE_RESIZE_IMPLEMENTED "Define this if you already have implemented stb_image_resize2.h via STB_IMAGE_RESIZE_IMPLEMENTATION." OFF)

if(TXTR_COMP_RGBA)
    set(TXTR_COMP_SH_B "16")
    set(TXTR_COMP_SH_G "8")
    set(TXTR_COMP_SH_R "0")
    set(TXTR_COMP_SH_A "24")
elseif(TXTR_COMP_ARGB)
    set(TXTR_COMP_SH_B "24")
    set(TXTR_COMP_SH_G "16")
    set(TXTR_COMP_SH_R "8")
    set(TXTR_COMP_SH_A "0")
elseif(TXTR_COMP_ABGR)
    set(TXTR_COMP_SH_B "8")
    set(TXTR_COMP_SH_G "16")
    set(TXTR_COMP_SH_R "24")
    set(TXTR_COMP_SH_A "0")
else()
    set(TXTR_COMP_BGRA ON)
    set(TXTR_COMP_SH_B "0")
    set(TXTR_COMP_SH_G "8")
    set(TXTR_COMP_SH_R "16")
    set(TXTR_COMP_SH_A "24")
endif()

if(NOT STB_IMAGE_RESIZE_IMPLEMENTED)
    set(STB_IMAGE_RESIZE_IMPLEMENTED ON)
    set(STB_IMAGE_RESIZE_FILE ${PROJECT_SOURCE_DIR}/../extern/stb_helpers/stb_image_resize2_impl.c)
else()
    set(STB_IMAGE_RESIZE_FILE "")
endif()

option(TXTR_STATIC "Build static library." ON)
if(TXTR_STATIC)
    set(TXTR_BUILD_TYPE STATIC)
    set(TXTR_LINK_TYPE ARCHIVE)
    set(TXTR_IS_SHARED OFF)
else()
    set(TXTR_BUILD_TYPE SHARED)
    set(TXTR_LINK_TYPE LIBRARY)
    set(TXTR_IS_SHARED ON)
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/txtr_settings.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/txtr_settings.h.in" "${PROJECT_SOURCE_DIR}/include/configure/txtr_settings.h" @ONLY)
endif()

add_library(txtr ${TXTR_BUILD_TYPE}
    ${PROJECT_SOURCE_DIR}/include/txtr.h
    
    ${PROJECT_SOURCE_DIR}/src/txtr.c
    
    ${STB_IMAGE_RESIZE_FILE})

target_include_directories(txtr
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../extern/stb)

target_compile_features(txtr
    PRIVATE
        c_std_99
        c_function_prototypes
        c_variadic_macros)

set_target_properties(txtr
    PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON)

target_compile_definitions(txtr
    PUBLIC
        $<$<BOOL:${STBIR_SSE2}>:STBIR_SSE2>
        $<$<BOOL:${STBIR_AVX}>:STBIR_AVX>
        $<$<BOOL:${STBIR_AVX2}>:STBIR_AVX2>)

target_precompile_headers(txtr
    PUBLIC
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/txtr_version.h>"
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/txtr_settings.h>")

# stdext
if(NOT TARGET stdext)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../extern/stdext CMAKE/extern/stdext)
endif()
target_link_libraries(txtr PUBLIC stdext)

# gxtexture
if(NOT TARGET gxtexture)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../gxtexture CMAKE/gxtexture)
endif()
target_link_libraries(txtr PUBLIC gxtexture)

install(TARGETS txtr
    ${TXTR_LINK_TYPE} DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
