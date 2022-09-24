include(icm_testing)

CPMAddPackage(gh:iboB/doctest-util@0.0.5)

macro(xec_test test)
    add_doctest_lib_test(${test} xec ${ARGN})
endmacro()

xec_test(TaskExecutor t-TaskExecutor.cpp)
xec_test(TaskScheduling t-TaskScheduling.cpp)
