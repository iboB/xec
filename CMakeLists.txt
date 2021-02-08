cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(xec)

# cpm
include(./get_cpm.cmake)

#######################################
# cmake lib
CPMAddPackage(
    NAME icm
    GITHUB_REPOSITORY iboB/icm
    VERSION 1.1.0
)
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${icm_SOURCE_DIR}
)

include(icm_dev_mode)
include(icm_add_lib)

#######################################
# options

option(XEC_STATIC "xec: build as static lib" OFF)
option(XEC_BUILD_TESTS "xec: build tests" ${ICM_DEV_MODE})

#######################################
# packages
CPMAddPackage(
    NAME splat
    GITHUB_REPOSITORY iboB/splat
    VERSION 1.0.0
)
CPMAddPackage(
    NAME itlib
    GITHUB_REPOSITORY iboB/itlib
    VERSION 1.2.2
)

find_package(Threads REQUIRED)

#######################################
# subdirs

add_subdirectory(code/xec)

if(XEC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
