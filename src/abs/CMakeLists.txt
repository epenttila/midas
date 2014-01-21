project(abs)

add_executable(abs main.cpp)
target_link_libraries(abs ${Boost_LIBRARIES} abslib lutlib evallib)
