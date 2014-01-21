project(purify)

set(purify_SOURCES
    main.cpp
)

add_executable(purify ${purify_SOURCES})
target_link_libraries(purify ${Boost_LIBRARIES} cfrlib)
