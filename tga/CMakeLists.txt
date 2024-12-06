# tga cmake list

cmake_minimum_required(VERSION 3.28, FATAL_ERROR)

project(tga LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../extern/cmake-modules")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_TAG --tags)
git_local_changes(GIT_LOCAL_CHANGES)

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/tga_version.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/tga_version.h.in" "${PROJECT_SOURCE_DIR}/include/configure/tga_version.h" @ONLY)
endif()

option(TGA_INCLUDE_ERROR_STRINGS "Include extended error strings within tga." OFF)
option(TGA_TEST_ALIGNMENT "Use void pointer cast instead of memcpy in certain functions to avoid hiding alignment errors." OFF)
option(TGA_INCLUDE_DECODE "Include decoding capabilities within tga." ON)
option(TGA_INCLUDE_ENCODE "Include encoding capabilities within tga." ON)

option(TGA_STATIC "Build static library." ON)
if(TGA_STATIC)
    set(TGA_BUILD_TYPE STATIC)
    set(TGA_LINK_TYPE ARCHIVE)
else()
    set(TGA_BUILD_TYPE SHARED)
    set(TGA_LINK_TYPE LIBRARY)
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/include/configure/tga_settings.h")
    configure_file("${PROJECT_SOURCE_DIR}/include/configure/tga_settings.h.in" "${PROJECT_SOURCE_DIR}/include/configure/tga_settings.h" @ONLY)
endif()

add_library(tga ${TGA_BUILD_TYPE}
    ${PROJECT_SOURCE_DIR}/include/tga.h
    
    ${PROJECT_SOURCE_DIR}/src/tga.c)

target_include_directories(tga
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

target_compile_features(tga
    PRIVATE
        c_std_99
        c_function_prototypes
        c_variadic_macros)

set_target_properties(tga
    PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON)

target_precompile_headers(tga
    PUBLIC
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/tga_version.h>"
        "$<$<COMPILE_LANGUAGE:C>:${PROJECT_SOURCE_DIR}/include/configure/tga_settings.h>")

# stdext
if (NOT TARGET stdext)
    add_subdirectory(${PROJECT_SOURCE_DIR}/../extern/stdext CMAKE/extern/stdext)
endif()
target_link_libraries(tga PUBLIC stdext)

install(TARGETS tga
    ${TGA_LINK_TYPE} DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
