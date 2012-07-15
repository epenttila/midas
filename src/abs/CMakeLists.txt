project(abs)

add_executable(abs main.cpp)
target_link_libraries(abs ${Boost_LIBRARIES} abslib lutlib evallib)

file(COPY ${evallib_SOURCE_DIR}/ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_preflop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_flop_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_turn_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${lutlib_SOURCE_DIR}/holdem_river_lut.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
