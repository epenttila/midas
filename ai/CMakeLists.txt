add_executable(ai abstraction.cpp evaluator.cpp holdem_game.cpp holdem_state.cpp kuhn_game.cpp kuhn_state.cpp main.cpp)
file(INSTALL ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
