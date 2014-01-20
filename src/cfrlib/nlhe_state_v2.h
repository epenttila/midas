#pragma once

#include "nlhe_state_base.h"
#include <boost/noncopyable.hpp>
#include <array>

template<int BITMASK>
class nlhe_state_v2 : public nlhe_state_base, private boost::noncopyable
{
public:
    static const int ACTIONS = detail::count_bits<BITMASK>::value;
    static std::array<int, nlhe_state_base::MAX_ACTIONS> action_to_index_;
    static std::array<holdem_action, ACTIONS> index_to_action_;

    nlhe_state_v2(const int stack_size);
    virtual ~nlhe_state_v2();
    int get_action_index() const;
    int get_round() const;
    const nlhe_state_v2* get_parent() const;
    bool is_terminal() const;
    const nlhe_state_v2* get_child(int index) const;
    int get_id() const;
    int get_player() const;
    int get_terminal_ev(int result) const;
    int get_child_count() const;
    const nlhe_state_v2* call() const;
    const nlhe_state_v2* raise(double fraction) const;
    std::array<int, 2> get_pot() const;
    int get_action_count() const;
    holdem_action get_action(int index) const;
    std::string get_action_name(holdem_action action) const;
    int get_stack_size() const;

private:
    nlhe_state_v2(const nlhe_state_v2* parent, const int action_index, const int player,
        const std::array<int, 2>& pot, const int round, const std::array<int, 2> raise_masks, int* id);
    void create_child(int action_index, int* id);
    bool is_raise(holdem_action action) const;
    bool is_action_enabled(holdem_action action) const;
    holdem_action get_max_action() const;
    int get_new_player_pot(int player_pot, int to_call, int in_pot, holdem_action action) const;
    void print(std::ostream& os) const;

    const int id_;
    const nlhe_state_v2* parent_;
    std::array<nlhe_state_v2*, ACTIONS> children_;
    const int action_index_;
    const int player_;
    const std::array<int, 2> pot_;
    const int round_;
    int child_count_;
    const int stack_size_;
    const std::array<int, 2> raise_masks_;
};

#include "nlhe_state_v2.ipp"
