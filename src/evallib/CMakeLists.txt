project(evallib)

set(evallib_SOURCES
    holdem_evaluator.cpp
    holdem_evaluator.h
    kuhn_evaluator.cpp
    kuhn_evaluator.h
    twoplustwo_evaluator.cpp
    twoplustwo_evaluator.h
)

add_library(evallib ${evallib_SOURCES})

if(MSVC)
    set_target_properties(evallib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
