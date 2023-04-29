# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
icm_add_lib(xec XEC
    ExecutionContext.hpp
    ExecutorBase.cpp
    ExecutorBase.hpp
    TaskExecutor.cpp
    TaskExecutor.hpp
    ThreadExecution.cpp
    ThreadExecution.hpp
    PoolExecution.cpp
    PoolExecution.hpp
    ThreadName.hpp
    ThreadName.cpp
)

add_library(xec::xec ALIAS xec)
target_include_directories(xec INTERFACE ..)
target_link_libraries(xec PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    splat::splat
    itlib::itlib
)
