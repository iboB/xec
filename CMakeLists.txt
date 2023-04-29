# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(xec
    LANGUAGES CXX
)

# cpm
include(./get_cpm.cmake)

#######################################
# cmake lib
CPMAddPackage(gh:iboB/icm@1.4.5)

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
CPMAddPackage(gh:iboB/splat@1.3.1)
CPMAddPackage(gh:iboB/itlib@1.9.1)

find_package(Threads REQUIRED)

#######################################
# subdirs

add_subdirectory(code/xec)

if(XEC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
