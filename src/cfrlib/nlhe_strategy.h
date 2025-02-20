#pragma once

#include <string>
#include <memory>

#include "gamelib/nlhe_state.h"
#include "abslib/holdem_abstraction_base.h"
#include "strategy.h"

class nlhe_strategy
{
public:
    nlhe_strategy(const std::string& filepath, bool read_only = true);
    const holdem_abstraction_base& get_abstraction() const;
    const nlhe_state& get_root_state() const;
    const strategy& get_strategy() const;
    strategy& get_strategy();
    int get_stack_size() const;

private:
    std::unique_ptr<nlhe_state> root_state_;
    std::unique_ptr<holdem_abstraction_base> abstraction_;
    std::unique_ptr<strategy> strategy_;
    int stack_size_;
};
