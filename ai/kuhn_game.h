#pragma once

#include "game.h"
#include "kuhn_state.h"

class kuhn_game : public game<kuhn_state, kuhn_state::ACTIONS, 1>
{
public:
    virtual void solve(int iterations);
    virtual std::ostream& print(std::ostream&) const;
    virtual void save(std::ostream&) const;
    virtual void load(std::istream&);
};
