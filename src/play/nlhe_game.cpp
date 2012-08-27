#include "nlhe_game.h"
#include <numeric>
#include <cassert>
#include <boost/format.hpp>
#include "actor_base.h"
#include "util/partial_shuffle.h"
#include "util/card.h"
#include "evallib/holdem_evaluator.h"
#include "cfrlib/holdem_game.h"

namespace
{
    // TODO move output formatting to main.cpp
    std::string format_chips(int x)
    {
        return (boost::format("$%1%") % x).str();
    }
}

nlhe_game::nlhe_game(const int stack_size, actor_base& a1, actor_base& a2)
    : actor1_(a1)
    , actor2_(a2)
    , engine_(std::random_device()())
    , dealer_(0)
    , stack_size_(stack_size)
    , bankroll_(0)
    , allin_(false)
    , eval_(new holdem_evaluator)
{
    a1.set_id(0);
    a2.set_id(1);
    names_[0] = "Alice";
    names_[1] = "Bob";
}

void nlhe_game::set_seed(unsigned long seed)
{
    engine_.seed(seed);
    std::iota(deck_.begin(), deck_.end(), 0);
}

void nlhe_game::play(std::size_t game)
{
    log_ << "Hand #" << game << "\n";

    bets_[dealer_] = 1;
    bets_[dealer_ ^ 1] = 2; // sb-bb

    log_ << "Seat 1: " << names_[0] << " (" << format_chips(stack_size_) << " in chips)\n";
    log_ << "Seat 2: " << names_[1] << " (" << format_chips(stack_size_) << " in chips)\n";
    log_ << names_[dealer_] << ": posts small blind " << format_chips(bets_[dealer_]) << "\n";
    log_ << names_[dealer_ ^ 1] << ": posts big blind " << format_chips(bets_[dealer_ ^ 1]) << "\n";

    partial_shuffle(deck_, 9, engine_); // 4 hole, 5 board

    std::array<std::pair<int, int>, 2> holes;

    holes[dealer_].first = deck_[deck_.size() - 1];
    holes[dealer_].second = deck_[deck_.size() - 2];
    holes[dealer_ ^ 1].first = deck_[deck_.size() - 3];
    holes[dealer_ ^ 1].second = deck_[deck_.size() - 4];
    const int c0 = holes[0].first;
    const int c1 = holes[0].second;
    const int d0 = holes[1].first;
    const int d1 = holes[1].second;
    const int b0 = deck_[deck_.size() - 5];
    const int b1 = deck_[deck_.size() - 6];
    const int b2 = deck_[deck_.size() - 7];
    const int b3 = deck_[deck_.size() - 8];
    const int b4 = deck_[deck_.size() - 9];

    int value1 = eval_->get_hand_value(c0, c1, b0, b1, b2, b3, b4);
    int value2 = eval_->get_hand_value(d0, d1, b0, b1, b2, b3, b4);
    result_ = value1 > value2 ? 1 : (value1 < value2 ? -1 : 0);

    actor1_.set_cards(c0, c1, b0, b1, b2, b3, b4);
    actor2_.set_cards(d0, d1, b0, b1, b2, b3, b4);

    log_ << "*** HOLE CARDS ***\n";
    log_ << "Dealt to " << names_[0] << " [" << get_card_string(c0) << " " << get_card_string(c1) << "]\n";
    log_ << "Dealt to " << names_[1] << " [" << get_card_string(d0) << " " << get_card_string(d1) << "]\n";

    running_ = true;
    player_ = dealer_;
    round_ = 0;
    int last_round = -1;
    pot_ = 0;
    allin_ = false;

    while (running_)
    {
        if (round_ > last_round)
        {
            switch (round_)
            {
            case holdem_game::FLOP:
                log_ << "*** FLOP *** [" << get_card_string(b0) << " " << get_card_string(b1) << " " << get_card_string(b2) << "]\n";
                break;
            case holdem_game::TURN:
                log_ << "*** TURN *** [" << get_card_string(b0) << " " << get_card_string(b1) << " " << get_card_string(b2) << "] [" << get_card_string(b3) << "]\n";
                break;
            case holdem_game::RIVER:
                log_ << "*** RIVER *** [" << get_card_string(b0) << " " << get_card_string(b1) << " " << get_card_string(b2) << " " << get_card_string(b3) << "] [" << get_card_string(b4) << "]\n";
                break;
            }

            last_round = round_;
        }

        //log_ << "Pot: " << pot_ << "\n";
        //log_ << "Bets: " << bets_[0] << " - " << bets_[1] << "\n";
        if (!allin_)
        {
            if (player_ == 0)
                actor1_.act(*this);
            else
                actor2_.act(*this);
        }
        else
        {
            ++round_;
        }

        if (round_ >= 4)
            running_ = false;

        log_.flush();
    }

    if (round_ == 4)
        showdown();

    //log_ << "Pot: " << pot_ << "\n";
    //log_ << "Bets: " << bets_[0] << " - " << bets_[1] << "\n";
    log_ << "BR: " << bankroll_ << "\n";
    log_ << "\n\n\n";
}

void nlhe_game::fold()
{
    log_ << names_[player_] << ": folds\n";
    bankroll_ += (player_ == 0 ? -1 : 1) * (pot_ / 2 + bets_[player_]);
    running_ = false;
    dealer_ = dealer_ ^ 1;
    log_ << names_[player_ ^ 1] << " collected " << format_chips(pot_ + bets_[player_])  << " from pot\n";
}

void nlhe_game::call()
{
    const int this_player = player_;
    int to_call = bets_[player_ ^ 1] - bets_[player_];
    bool closing = false;
    bets_[player_] = bets_[player_ ^ 1];
    assert(round_ > 0 || bets_[dealer_ ^ 1] > 1);
    allin_ = (pot_ / 2 + bets_[player_] == stack_size_);

    if (to_call == 0)
        log_ << names_[player_] << ": checks\n";
    else
        log_ << names_[player_] << ": calls " << format_chips(to_call) << (allin_ ? " and is all-in" : "") << "\n";

    if (round_ == 0)
    {
        if (player_ == dealer_ && to_call == 1)
            closing = false;
        else if (to_call > 1)
            closing = true;
        else if (to_call == 0 && player_ != dealer_)
            closing = true;
    }
    else
    {
        if (to_call > 0 || player_ == dealer_)
            closing = true;
    }

    assert(pot_ / 2 + bets_[player_] <= stack_size_);

    if (closing)
    {
        pot_ += bets_[0] + bets_[1];
        assert(bets_[0] == bets_[1]);
        bets_.fill(0);
    }

    // new player
    if (this_player == 0)
        actor2_.opponent_called(*this);
    else
        actor1_.opponent_called(*this);

    if (closing)
    {
        ++round_;
        player_ = dealer_ ^ 1;
    }
    else
    {
        player_ = player_ ^ 1;
    }
}

void nlhe_game::raise(int amount)
{
    const int this_player = player_;
    const int to_call = bets_[player_ ^ 1] - bets_[player_];
    int min_raise = std::max(2, to_call);
    //assert(amount >= min_raise);

    if (amount < min_raise)
    {
        call();
        return;
    }

    if (to_call + bets_[player_] + pot_ / 2 == stack_size_)
    {
        call();
        return;
    }

    int max_raise = stack_size_ - pot_ / 2 - bets_[player_ ^ 1];

    amount = std::min(amount, max_raise);

    bool allin = (pot_ / 2 + bets_[player_ ^ 1] + amount >= stack_size_);

    if (allin)
        amount = stack_size_ - pot_ / 2 - bets_[player_ ^ 1];

    const int total_bet = bets_[player_ ^ 1] + amount;

    assert(total_bet + pot_ / 2 <= stack_size_);

    if (to_call == 0 && round_ > 0)
        log_ << names_[player_] << ": bets " << format_chips(amount);
    else
        log_ << names_[player_] << ": raises " << format_chips(amount) << " to " << format_chips(total_bet);

    if (allin)
        log_ << " and is all-in";

    log_ << "\n";

    bets_[player_] = total_bet;
    player_ = player_ ^ 1;

    // new player
    if (this_player == 0)
        actor2_.opponent_raised(*this, amount);
    else
        actor1_.opponent_raised(*this, amount);

#if !defined(NDEBUG)
    /*assert(actor1_.state_->get_id() == actor2_.state_->get_id());
    auto p1 = actor1_.state_->get_pot();
    auto p2 = actor2_.state_->get_pot();
    assert(pot_[dealer_] == p1[0] && pot_[dealer_ ^ 1] == p1[1]);
    assert(pot_[dealer_] == p2[0] && pot_[dealer_ ^ 1] == p2[1]);*/
#endif
}

void nlhe_game::showdown()
{
    assert(bets_[0] == 0 && bets_[1] == 0);
    bankroll_ += result_ * pot_ / 2;
    running_ = false;
    dealer_ = dealer_ ^ 1;

    if (log_)
    {
        log_ << "*** SHOW DOWN ***\n";

        if (result_ == 1)
            log_ << names_[0] << " collected " << format_chips(pot_) << " from pot\n";
        else if (result_ == -1)
            log_ << names_[1] << " collected " << format_chips(pot_) << " from pot\n";
        else
        {
            log_ << names_[0] << " collected " << format_chips(pot_ / 2) << " from pot\n";
            log_ << names_[1] << " collected " << format_chips(pot_ / 2) << " from pot\n";
        }
    }
}
