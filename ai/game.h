#pragma once

#include "cfr_solver.h"

class game_base
{
    friend std::ostream& operator<<(std::ostream& o, const game_base& game);

public:
    virtual void solve(int) = 0;
    virtual std::ostream& print(std::ostream&) const = 0;
    virtual void save(std::ostream&) const = 0;
    virtual void load(std::istream&) = 0;
};

inline std::ostream& operator<<(std::ostream& o, const game_base& game)
{
    return game.print(o);
}

template<class game_state, int num_actions, int num_rounds>
class game : public game_base
{
public:
    typedef cfr_solver<game_state, num_actions, num_rounds> solver_t;

    game()
    {
        generate(new game_state);
    }

protected:
    void generate(game_state* state)
    {
        if (!state->is_terminal())
            states_.push_back(state);

        for (int i = 0; i < num_actions; ++i)
        {
            if (game_state* next = state->act(i, int(states_.size())))
                generate(next);
        }
    }

    std::unique_ptr<solver_t> solver_;
    std::vector<game_state*> states_;
};
