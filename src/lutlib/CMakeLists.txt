project(lutlib)

set(lutlib_SOURCES
    holdem_flop_lut.cpp
    holdem_preflop_lut.cpp
    holdem_river_lut.cpp
    holdem_turn_lut.cpp
)

add_library(lutlib ${lutlib_SOURCES})
