project(gamelib)

set(SOURCES
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
    nlhe_state.h
    nlhe_state.cpp
    nlhe_state_base.h
    nlhe_state_base.cpp
    game_state_base.h
    game_state_base.cpp
)

add_library(gamelib ${SOURCES})
target_link_libraries(gamelib)

if(MSVC)
    set_target_properties(gamelib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
