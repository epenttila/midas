project(lutlib)

set(lutlib_SOURCES
    holdem_flop_lut.cpp
    holdem_preflop_lut.cpp
    holdem_river_lut.cpp
    holdem_turn_lut.cpp
)

add_library(lutlib ${lutlib_SOURCES})

if(MSVC)
    set_target_properties(lutlib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
