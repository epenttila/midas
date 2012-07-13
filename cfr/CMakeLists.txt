set(CFR_SOURCES
    cfr_solver.cpp
    holdem_abstraction.cpp
    holdem_evaluator.cpp
    holdem_flop_lut.cpp
    holdem_game.cpp
    holdem_preflop_lut.cpp
    holdem_river_lut.cpp
    holdem_state.cpp
    holdem_turn_lut.cpp
    kuhn_abstraction.cpp
    kuhn_evaluator.cpp
    kuhn_game.cpp
    kuhn_state.cpp
    main.cpp
	nl_holdem_state.cpp
    twoplustwo_evaluator.cpp
)

set(LUT_SOURCES
    holdem_evaluator.cpp
    holdem_flop_lut.cpp
    holdem_preflop_lut.cpp
    holdem_river_lut.cpp
    holdem_turn_lut.cpp
    lut.cpp
    twoplustwo_evaluator.cpp
)

add_executable(cfr ${CFR_SOURCES})
target_link_libraries(cfr ${Boost_LIBRARIES})

add_executable(lut ${LUT_SOURCES})

file(INSTALL ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(gui_SOURCES
    gui.cpp
    gui_main.cpp
)

set(gui_HEADERS
    gui.h
)

qt4_wrap_cpp(gui_HEADERS_MOC ${gui_HEADERS})

add_executable(gui WIN32 ${gui_SOURCES} ${gui_HEADERS_MOC})
target_link_libraries(gui ${QT_LIBRARIES})
