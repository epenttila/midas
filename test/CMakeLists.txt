project(test)

set(test_SOURCES
    nlhe_state_v2_tests.cpp
    binary_io_tests.cpp
)

add_executable(testrunner ${test_SOURCES})
target_link_libraries(testrunner ${Boost_LIBRARIES} gtest gtest_main cfrlib)

add_test(AllTests testrunner --gtest_output=xml)
