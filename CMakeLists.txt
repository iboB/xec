cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(xec)

# cpm
include(./get_cpm.cmake)

#######################################
# cmake lib
CPMAddPackage(gh:iboB/icm@1.3.4)

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
CPMAddPackage(gh:iboB/splat@1.2.1)
CPMAddPackage(gh:iboB/itlib@1.6.3)

find_package(Threads REQUIRED)

#######################################
# subdirs

add_subdirectory(code/xec)

if(XEC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
