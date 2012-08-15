project(gui)

set(gui_SOURCES
    holdem_strategy_widget.cpp
    holdem_strategy_widget.h
    input_manager.cpp
    input_manager.h
    lobby_888.cpp
    lobby_888.h
    lobby_base.cpp
    lobby_base.h
    main.cpp
    main_window.cpp
    main_window.h
    settings_dialog.cpp
    settings_dialog.h
    site_888.cpp
    site_888.h
    site_base.cpp
    site_base.h
    site_stars.cpp
    site_stars.h
    table_widget.cpp
    table_widget.h
    window_manager.cpp
    window_manager.h
)

set(gui_HEADERS
    holdem_strategy_widget.h
    main_window.h
    settings_dialog.h
    table_widget.h
)

set(gui_RESOURCES
    gui.qrc
)

qt4_wrap_cpp(gui_HEADERS_MOC ${gui_HEADERS})
qt4_add_resources(gui_RESOURCES_RCC ${gui_RESOURCES})
add_executable(gui WIN32 ${gui_SOURCES} ${gui_HEADERS_MOC} ${gui_RESOURCES_RCC})
target_link_libraries(gui ${Boost_LIBRARIES} ${QT_LIBRARIES} cfrlib lutlib evallib abslib)

file(COPY ${evallib_SOURCE_DIR}/ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_preflop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_flop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_turn_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_river_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
