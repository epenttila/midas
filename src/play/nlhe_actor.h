#pragma once

#include <string>
#include <memory>
#include "actor_base.h"
#include "abslib/holdem_abstraction.h"

class nlhe_strategy;
class nlhe_state_base;

class nlhe_actor : public actor_base
{
public:
    nlhe_actor(const std::string& filename);
    void set_cards(int c0, int c1, int b0, int b1, int b2, int b3, int b4);
    void act(nlhe_game& g);
    void opponent_called(const nlhe_game& g);
    void opponent_raised(const nlhe_game& g, int amount);
    void set_id(int id) { id_ = id; }

private:
    const nlhe_state_base* state_;
    std::unique_ptr<nlhe_strategy> strategy_;
    holdem_abstraction::bucket_type buckets_;
    int id_;
};
