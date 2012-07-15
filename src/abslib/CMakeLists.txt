project(abslib)

set(abslib_SOURCES
    holdem_abstraction.cpp
    kuhn_abstraction.cpp
)

add_library(abslib ${abslib_SOURCES})
