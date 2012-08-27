#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <array>
#include <random>
#include <fstream>
#include <boost/noncopyable.hpp>

class actor_base;
class holdem_evaluator;

class nlhe_game : private boost::noncopyable
{
public:
    nlhe_game(int stack_size, actor_base& a1, actor_base& a2);
    void play(std::size_t game);
    void fold();
    void call();
    void raise(int amount);
    void showdown();
    int get_round() const { return round_; }
    int get_to_call(int id) const { return bets_[id ^ 1] - bets_[id]; }
    int get_pot(int) const { return pot_ + bets_[0] + bets_[1]; }
    int get_bankroll() const { return bankroll_; }
    void set_seed(unsigned long seed);
    int get_bets(int id) const { return bets_[id]; }
    void set_log(const std::string& filename) { log_.open(filename); }
    void set_dealer(int dealer) { dealer_ = dealer; }

private:
    actor_base& actor1_;
    actor_base& actor2_;
    bool running_;
    int round_;
    int player_;
    int pot_;
    int bankroll_;
    std::unique_ptr<holdem_evaluator> eval_;
    int dealer_;
    std::array<int, 52> deck_;
    std::mt19937 engine_;
    int result_;
    int stack_size_;
    std::array<int, 2> bets_;
    std::array<std::string, 2> names_;
    std::ofstream log_;
    bool allin_;
};
