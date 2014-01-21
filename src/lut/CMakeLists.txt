project(lut)

add_executable(lut main.cpp)
target_link_libraries(lut ${Boost_LIBRARIES} lutlib evallib)
