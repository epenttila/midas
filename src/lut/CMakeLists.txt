project(lut)

add_executable(lut main.cpp)
target_link_libraries(lut ${Boost_LIBRARIES} lutlib evallib)

file(COPY ${evallib_SOURCE_DIR}/ranks.dat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
