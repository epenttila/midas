project(play)

set(play_SOURCES
    main.cpp
)

add_executable(play ${play_SOURCES})
target_link_libraries(play ${Boost_LIBRARIES} cfrlib)
