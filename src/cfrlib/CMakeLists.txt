project(cfrlib)

set(cfrlib_SOURCES
    cfr_solver.cpp
    holdem_game.cpp
    holdem_state.cpp
    kuhn_game.cpp
    kuhn_state.cpp
    nl_holdem_state.cpp
)

add_library(cfrlib ${cfrlib_SOURCES})
