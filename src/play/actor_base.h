#pragma once

#include <boost/noncopyable.hpp>

class nlhe_game;

class actor_base : private boost::noncopyable
{
public:
    actor_base() {}
    virtual ~actor_base() {}
    virtual void set_cards(int c0, int c1, int b0, int b1, int b2, int b3, int b4) = 0;
    virtual void act(nlhe_game& g) = 0;
    virtual void opponent_called(const nlhe_game& g) = 0;
    virtual void opponent_raised(const nlhe_game& g, int amount) = 0;
    virtual void set_id(int id) = 0;
};
