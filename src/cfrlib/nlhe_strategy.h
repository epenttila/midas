#pragma once

#include <string>
#include <memory>

class nlhe_state_base;
class holdem_abstraction_base;
class strategy;

class nlhe_strategy
{
public:
    nlhe_strategy(const std::string& filepath);
    const holdem_abstraction_base& get_abstraction() const;
    const nlhe_state_base& get_root_state() const;
    const strategy& get_strategy() const;
    int get_stack_size() const;

private:
    std::unique_ptr<nlhe_state_base> root_state_;
    std::unique_ptr<holdem_abstraction_base> abstraction_;
    std::unique_ptr<strategy> strategy_;
    int stack_size_;
};
