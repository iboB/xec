include(icm_testing)

CPMAddPackage(
    NAME doctest-lib
    GITHUB_REPOSITORY iboB/doctest-lib
    GIT_TAG 2.4.3b
)

macro(xec_test test)
    icm_add_doctest_lib_test(${test} xec ${ARGN})
endmacro()

xec_test(TaskExecutor t-TaskExecutor.cpp)
xec_test(TaskScheduling t-TaskScheduling.cpp)
