project(gamelib)

set(SOURCES
    holdem_dealer.h
    flhe_state.cpp
    flhe_state.h
    kuhn_dealer.cpp
    kuhn_dealer.h
    kuhn_state.cpp
    kuhn_state.h
    leduc_dealer.cpp
    leduc_dealer.h
    leduc_state.cpp
    leduc_state.h
    nlhe_state.h
    nlhe_state.cpp
    game_state_base.h
    game_state_base.cpp
    holdem_state.h
    holdem_dealer.cpp
)

add_library(gamelib ${SOURCES})
target_link_libraries(gamelib evallib)

if(MSVC)
    set_target_properties(gamelib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
