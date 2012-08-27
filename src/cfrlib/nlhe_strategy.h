#pragma once

#include <string>
#include <memory>

class nlhe_state_base;
class holdem_abstraction;
class strategy;

class nlhe_strategy
{
public:
    nlhe_strategy(const std::string& filepath);
    const holdem_abstraction& get_abstraction() const;
    const nlhe_state_base& get_root_state() const;
    const strategy& get_strategy() const;

private:
    std::unique_ptr<nlhe_state_base> root_state_;
    std::unique_ptr<holdem_abstraction> abstraction_;
    std::unique_ptr<strategy> strategy_;
};
