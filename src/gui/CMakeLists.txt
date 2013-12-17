project(gui)

set(gui_SOURCES
    holdem_strategy_widget.cpp
    holdem_strategy_widget.h
    input_manager.cpp
    input_manager.h
    lobby_manager.cpp
    lobby_manager.h
    main.cpp
    main_window.cpp
    main_window.h
    table_manager.cpp
    table_manager.h
    state_widget.cpp
    state_widget.h
    table_widget.cpp
    table_widget.h
    window_utils.cpp
    window_utils.h
    qt_log_sink.cpp
    qt_log_sink.h
    fake_window.cpp
    fake_window.h
    smtp.cpp
    smtp.h
)

set(gui_RESOURCES
    gui.qrc
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

qt5_add_resources(gui_RESOURCES_RCC ${gui_RESOURCES})
add_executable(gui WIN32 ${gui_SOURCES} ${gui_RESOURCES_RCC})
target_link_libraries(gui ${Boost_LIBRARIES} Qt5::Widgets Qt5::Network cfrlib lutlib evallib abslib)

file(COPY ${evallib_SOURCE_DIR}/ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_preflop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_flop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_turn_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_river_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
