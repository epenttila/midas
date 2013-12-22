project(util)

set(util_SOURCES
    binary_io.h
    card.h
    choose.h
    holdem_loops.h
    k_means.h
    partial_shuffle.h
    sort.h
    metric.h
    game.h
    random.h
)

add_custom_target(util SOURCES ${util_SOURCES})
