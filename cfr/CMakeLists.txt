set(SOURCES
    holdem_abstraction.cpp
    holdem_evaluator.cpp
    holdem_game.cpp
    holdem_state.cpp
    kuhn_abstraction.cpp
    kuhn_evaluator.cpp
    kuhn_game.cpp
    kuhn_state.cpp
    main.cpp
)

set(HEADERS
    cfr_solver.h
	common.h
    holdem_abstraction.h
    holdem_evaluator.h
    holdem_game.h
    holdem_state.h
    kuhn_abstraction.h
    kuhn_evaluator.h
    kuhn_game.h
    kuhn_state.h
)

add_executable(cfr ${SOURCES} ${HEADERS})
file(INSTALL ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cfr ${Boost_LIBRARIES})
