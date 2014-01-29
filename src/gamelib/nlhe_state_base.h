#pragma once

#include <memory>
#include <string>
#include <array>
#include <vector>
#include "holdem_state.h"

namespace detail
{
    static const std::array<int, 2> INITIAL_POT = {{1, 2}};

    template<unsigned int N>
    struct count_bits
    {
        static const unsigned int value = count_bits<(N >> 1)>::value + (N % 2);
    };

    template<>
    struct count_bits<0>
    {
        static const unsigned int value = 0;
    };
}

class nlhe_state_base : public holdem_state
{
public:
    enum
    {
        F_MASK = 0x001,
        C_MASK = 0x002,
        O_MASK = 0x004,
        H_MASK = 0x008,
        Q_MASK = 0x010,
        P_MASK = 0x020,
        W_MASK = 0x040,
        D_MASK = 0x080,
        V_MASK = 0x100,
        T_MASK = 0x200,
        A_MASK = 0x400,
    };

    enum holdem_action
    {
        INVALID_ACTION = -1,
        FOLD = 0,
        CALL = 1,
        RAISE_O = 2,
        RAISE_H = 3,
        RAISE_Q = 4,
        RAISE_P = 5,
        RAISE_W = 6,
        RAISE_D = 7,
        RAISE_V = 8,
        RAISE_T = 9,
        RAISE_A = 10,
        ACTIONS,
    };

    static int get_action_mask(holdem_action action);
    static double get_raise_factor(const holdem_action action);
    static std::string get_action_name(const holdem_action action);

    virtual const nlhe_state_base* call() const = 0;
    virtual const nlhe_state_base* raise(double fraction) const = 0;
    virtual game_round get_round() const = 0;
    virtual int get_id() const = 0;
    virtual std::array<int, 2> get_pot() const = 0;
    virtual bool is_terminal() const = 0;
    virtual const nlhe_state_base* get_child(int index) const = 0;
    virtual const nlhe_state_base* get_action_child(int action) const = 0;
    virtual holdem_action get_action() const = 0;
    virtual int get_player() const = 0;
    virtual int get_stack_size() const = 0;
    virtual const nlhe_state_base* get_parent() const = 0;

protected:
    static int soft_translate(const double b1, const double b, const double b2);
    static const nlhe_state_base* raise(const nlhe_state_base& state, double fraction);
    static int get_new_player_pot(const int player_pot, const int to_call, const int in_pot, 
        const holdem_action action, int stack_size);
};

std::ostream& operator<<(std::ostream& os, const nlhe_state_base& state);
