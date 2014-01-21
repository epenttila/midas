project(play)

set(play_SOURCES
    actor_base.h
    always_call_actor.cpp
    always_call_actor.h
    always_fold_actor.cpp
    always_fold_actor.h
    always_raise_actor.cpp
    always_raise_actor.h
    game_base.h
    main.cpp
    nlhe_actor.cpp
    nlhe_actor.h
    nlhe_game.cpp
    nlhe_game.h
)

add_executable(play ${play_SOURCES})
target_link_libraries(play ${Boost_LIBRARIES} cfrlib lutlib evallib abslib)
