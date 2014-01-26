project(lutlib)

set(lutlib_SOURCES
    holdem_river_lut.cpp
    holdem_river_lut.h
    hand_indexer.cpp
    hand_indexer.h
    holdem_river_ochs_lut.cpp
    holdem_river_ochs_lut.h
)

add_library(lutlib ${lutlib_SOURCES})
target_link_libraries(lutlib evallib util)

if(MSVC)
    set_target_properties(lutlib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()
