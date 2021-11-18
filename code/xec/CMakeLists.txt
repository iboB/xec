icm_add_lib(xec XEC
    ExecutionContext.hpp
    ExecutorBase.cpp
    ExecutorBase.hpp
    TaskExecutor.cpp
    TaskExecutor.hpp
    ThreadExecution.cpp
    ThreadExecution.hpp
    ThreadName.hpp
    ThreadName.cpp
)

target_include_directories(xec INTERFACE ..)
target_link_libraries(xec PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    splat
    itlib
)
