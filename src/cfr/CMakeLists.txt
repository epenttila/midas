project(cfr)

add_executable(cfr main.cpp)
target_link_libraries(cfr ${Boost_LIBRARIES} cfrlib lutlib evallib)
