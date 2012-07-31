project(cfrlib)

set(cfrlib_SOURCES
    holdem_game.cpp
    holdem_state.cpp
    kuhn_game.cpp
    kuhn_state.cpp
    strategy.cpp
)

add_library(cfrlib ${cfrlib_SOURCES})

if(MSVC)
    set_target_properties(cfrlib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
