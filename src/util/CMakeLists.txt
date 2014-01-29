project(util)

set(util_SOURCES
    binary_io.h
    binary_io.cpp
    card.h
    choose.h
    k_means.h
    partial_shuffle.h
    sort.h
    metric.h
    random.h
    version.h
)

list(APPEND util_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")

add_custom_command(
    OUTPUT fake-gen-version
    COMMAND ${CMAKE_COMMAND}
        -DGIT_EXECUTABLE="${GIT_EXECUTABLE}"
        -DINPUT="${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in"
        -DOUTPUT="${CMAKE_CURRENT_BINARY_DIR}/version.cpp"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

add_custom_command(
    OUTPUT version.cpp
    DEPENDS fake-gen-version)

add_library(util ${util_SOURCES})
