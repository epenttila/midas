project(test)

set(test_SOURCES
    nlhe_state_test.cpp
    binary_io_test.cpp
    sort_test.cpp
    holdem_evaluator_test.cpp
    holdem_river_lut_test.cpp
    holdem_river_ochs_lut_test.cpp
    config.h
    pure_cfr_solver_test.cpp
)

list(APPEND test_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/config.cpp")

add_executable(testrunner EXCLUDE_FROM_ALL ${test_SOURCES})
target_link_libraries(testrunner ${Boost_LIBRARIES} gtest gtest_main cfrlib)
add_custom_target(check-${PROJECT_NAME} testrunner DEPENDS testrunner)
add_dependencies(check check-${PROJECT_NAME})

add_test(AllTests testrunner --gtest_output=xml)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/config.cpp" @ONLY)
