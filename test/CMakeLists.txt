project(test)

set(test_SOURCES
    nlhe_state_v2_unittests.cpp
)

add_executable(test ${test_SOURCES})
target_link_libraries(test ${Boost_LIBRARIES} gtest gtest_main cfrlib)

add_test(AllTests test --gtest_output=xml)
