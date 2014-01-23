project(test)

set(test_SOURCES
    nlhe_state_test.cpp
    binary_io_test.cpp
)

add_executable(testrunner ${test_SOURCES})
target_link_libraries(testrunner ${Boost_LIBRARIES} gtest gtest_main cfrlib)

add_test(AllTests testrunner --gtest_output=xml)
