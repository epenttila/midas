#pragma once

#include "game.h"
#include "holdem_state.h"

class holdem_game : public game<holdem_state, holdem_state::ACTIONS, holdem_state::ROUNDS>
{
public:
    holdem_game();
    virtual void solve(int iterations);
    virtual std::ostream& print(std::ostream&) const;
    virtual void save(std::ostream&) const;
    virtual void load(std::istream&);

private:
    int iterations_;
};
