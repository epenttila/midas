project(gui)

set(gui_SOURCES
    main_window.cpp
    main.cpp
    site_stars.cpp
    table_widget.cpp
    strategy_dialog.cpp
)

set(gui_HEADERS
    main_window.h
    table_widget.h
    strategy_dialog.h
)

set(gui_RESOURCES
    gui.qrc
)

qt4_wrap_cpp(gui_HEADERS_MOC ${gui_HEADERS})
qt4_add_resources(gui_RESOURCES_RCC ${gui_RESOURCES})
add_executable(gui WIN32 ${gui_SOURCES} ${gui_HEADERS_MOC} ${gui_RESOURCES_RCC})
target_link_libraries(gui ${QT_LIBRARIES} cfrlib lutlib evallib abslib)

file(COPY ${evallib_SOURCE_DIR}/ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_preflop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_flop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_turn_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_river_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
