include(icm_testing)

CPMAddPackage(gh:iboB/doctest-lib@2.4.6)

macro(xec_test test)
    icm_add_doctest_lib_test(${test} xec ${ARGN})
endmacro()

xec_test(TaskExecutor t-TaskExecutor.cpp)
xec_test(TaskScheduling t-TaskScheduling.cpp)
