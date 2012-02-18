#pragma once

#include "cfr_solver.h"
#include "kuhn_state.h"

class kuhn_game : private boost::noncopyable
{
public:
    kuhn_game();
    void train(const int iterations);
    void print_strategy();

private:
    typedef cfr_solver<kuhn_state, kuhn_state::ACTIONS> solver_t;
    void generate_states();
    std::unique_ptr<solver_t> solver_;
    std::vector<kuhn_state*> states_;
};
