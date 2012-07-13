project(evallib)

set(evallib_SOURCES
    holdem_evaluator.cpp
    kuhn_evaluator.cpp
    twoplustwo_evaluator.cpp
)

add_library(evallib ${evallib_SOURCES})

file(COPY ranks.dat DESTINATION ${CMAKE_BINARY_DIR})
