project(cfrlib)

set(cfrlib_SOURCES
    cfr_solver.h
    cfr_solver.ipp
    holdem_game.h
    flhe_state.cpp
    flhe_state.h
    kuhn_game.cpp
    kuhn_game.h
    kuhn_state.cpp
    kuhn_state.h
    leduc_game.cpp
    leduc_game.h
    leduc_state.cpp
    leduc_state.h
    nlhe_state.cpp
    nlhe_state.h
    nlhe_state.ipp
    nlhe_strategy.h
    nlhe_strategy.cpp
    pcs_cfr_solver.h
    pcs_cfr_solver.ipp
    solver_base.h
    strategy.cpp
    strategy.h
    cs_cfr_solver.h
    cs_cfr_solver.ipp
    pure_cfr_solver.h
    pure_cfr_solver.ipp
)

add_library(cfrlib ${cfrlib_SOURCES})

if(MSVC)
    set_target_properties(cfrlib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
