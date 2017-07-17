project(cfrlib)

set(cfrlib_SOURCES
    cfr_solver.h
    cfr_solver.ipp
    nlhe_strategy.h
    nlhe_strategy.cpp
    solver_base.h
    strategy.cpp
    strategy.h
    pure_cfr_solver.h
    pure_cfr_solver.ipp
)

add_library(cfrlib ${cfrlib_SOURCES})
target_link_libraries(cfrlib evallib abslib util gamelib)

if(MSVC)
    set_target_properties(cfrlib PROPERTIES
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
        STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
endif()

add_library(py${PROJECT_NAME} SHARED py.cpp)
target_link_libraries(py${PROJECT_NAME} ${PROJECT_NAME} ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(py${PROJECT_NAME} PROPERTIES PREFIX "")
