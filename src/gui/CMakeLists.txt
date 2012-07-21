project(gui)

set(gui_SOURCES
    gui.cpp
    main.cpp
    site_stars.cpp
    table_widget.cpp
)

set(gui_HEADERS
    gui.h
    table_widget.h
)

qt4_wrap_cpp(gui_HEADERS_MOC ${gui_HEADERS})
add_executable(gui WIN32 ${gui_SOURCES} ${gui_HEADERS_MOC})
target_link_libraries(gui ${QT_LIBRARIES} cfrlib lutlib evallib abslib)

file(COPY ${evallib_SOURCE_DIR}/ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_preflop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_flop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_turn_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_river_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY cards.png DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY dealer.png DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
