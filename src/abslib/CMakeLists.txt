project(abslib)

set(abslib_SOURCES
    holdem_abstraction.cpp
    holdem_abstraction.h
    kuhn_abstraction.cpp
    kuhn_abstraction.h
    leduc_abstraction.cpp
    leduc_abstraction.h
    holdem_abstraction_base.h
)

add_library(abslib ${abslib_SOURCES})
target_link_libraries(abslib lutlib evallib util)

if(MSVC)
    set_target_properties(abslib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()

add_library(py${PROJECT_NAME} SHARED py.cpp)
target_link_libraries(py${PROJECT_NAME} ${PROJECT_NAME} ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(py${PROJECT_NAME} PROPERTIES PREFIX "")
