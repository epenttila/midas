#pragma once

#include <boost/noncopyable.hpp>
#include <array>

enum
{
    F_MASK = 0x001,
    C_MASK = 0x002,
    H_MASK = 0x004,
    Q_MASK = 0x008,
    P_MASK = 0x010,
    W_MASK = 0x020,
    D_MASK = 0x040,
    T_MASK = 0x080,
    E_MASK = 0x100,
    A_MASK = 0x200,
};

class nlhe_state_base
{
public:
    enum holdem_action
    {
        FOLD,
        CALL,
        RAISE_H,
        RAISE_Q,
        RAISE_P,
        RAISE_W,
        RAISE_D,
        RAISE_T,
        RAISE_E,
        RAISE_A,
        MAX_ACTIONS,
    };

    virtual const nlhe_state_base* call() const = 0;
    virtual const nlhe_state_base* raise(double fraction) const = 0;
    virtual int get_round() const = 0;
    virtual int get_id() const = 0;
    virtual std::array<int, 2> get_pot() const = 0;
    virtual bool is_terminal() const = 0;
    virtual const nlhe_state_base* get_child(int index) const = 0;
    virtual int get_action_count() const = 0;
    virtual int get_action(int index) const = 0;
    virtual void print(std::ostream& os) const = 0;
    virtual int get_action() const = 0;
    virtual int get_player() const = 0;
};

template<int BITMASK>
class nl_holdem_state : public nlhe_state_base, private boost::noncopyable
{
public:
    static const int ACTIONS = detail::count_bits<BITMASK>::value;
    static std::array<int, nlhe_state_base::MAX_ACTIONS> action_to_index_;
    static std::array<int, ACTIONS> index_to_action_;

    nl_holdem_state(const int stack_size);
    ~nl_holdem_state();
    int get_action() const;
    int get_round() const;
    const nl_holdem_state* get_parent() const;
    bool is_terminal() const;
    const nl_holdem_state* get_child(int index) const;
    int get_id() const;
    int get_player() const;
    int get_terminal_ev(int result) const;
    int get_child_count() const;
    const nl_holdem_state* call() const;
    const nl_holdem_state* raise(double fraction) const;
    std::array<int, 2> get_pot() const;
    int get_action_count() const;
    int get_action(int index) const;

private:
    nl_holdem_state(const nl_holdem_state* parent, const int action_index, const int player,
        const std::array<int, 2>& pot, const int round, int* id);
    void create_child(const int action_index, int* id);
    bool is_raise(int action) const;
    bool is_action_enabled(int action) const;
    int get_max_action() const;
    int get_new_player_pot(const int player_pot, const int to_call, const int in_pot, const int action) const;
    void print(std::ostream& os) const;

    const int id_;
    const nl_holdem_state* parent_;
    std::array<nl_holdem_state*, ACTIONS> children_;
    const int action_index_;
    const int player_;
    const std::array<int, 2> pot_;
    const int round_;
    int child_count_;
    const int stack_size_;
};

std::ostream& operator<<(std::ostream& os, const nlhe_state_base& state);

#include "nl_holdem_state.ipp"
