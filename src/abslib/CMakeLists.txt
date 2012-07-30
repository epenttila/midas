project(abslib)

set(abslib_SOURCES
    holdem_abstraction.cpp
    kuhn_abstraction.cpp
)

add_library(abslib ${abslib_SOURCES})

if(MSVC)
    set_target_properties(abslib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
