project(test)

set(test_SOURCES
    nlhe_state_test.cpp
    binary_io_test.cpp
    sort_test.cpp
    holdem_evaluator_test.cpp
    holdem_river_lut_test.cpp
)

add_executable(testrunner ${test_SOURCES})
target_link_libraries(testrunner ${Boost_LIBRARIES} gtest gtest_main cfrlib)

add_test(AllTests testrunner --gtest_output=xml)

configure_file("${TEST_DATA_PATH}/ranks.dat" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
configure_file("${TEST_DATA_PATH}/holdem_river_lut.dat" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
