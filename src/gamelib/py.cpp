#include <pybind11/pybind11.h>
#include "nlhe_state.h"

namespace py = pybind11;

PYBIND11_PLUGIN(pygamelib)
{
    py::module m("pygamelib", "midas Python plugin");

    py::class_<game_state_base>(m, "GameStateBase");
    py::class_<holdem_state, game_state_base>(m, "HoldemState");

    py::class_<nlhe_state, holdem_state> py_nlhe_state(m, "NLHEState");
    py_nlhe_state
        .def_static("get_raise_factor", &nlhe_state::get_raise_factor)
        .def_static("get_action_name", &nlhe_state::get_action_name)
        .def_property_readonly("action", &nlhe_state::get_action)
        .def_property_readonly("round", &nlhe_state::get_round)
        .def_property_readonly("parent", &nlhe_state::get_parent, py::return_value_policy::reference)
        .def_property_readonly("terminal", &nlhe_state::is_terminal)
        .def("get_child", &nlhe_state::get_child, py::return_value_policy::reference)
        .def("get_action_child", &nlhe_state::get_action_child, py::return_value_policy::reference)
        .def_property_readonly("id", &nlhe_state::get_id)
        .def_property_readonly("player", &nlhe_state::get_player)
        .def("get_terminal_ev", &nlhe_state::get_terminal_ev)
        .def_property_readonly("child_count", &nlhe_state::get_child_count)
        .def("call", &nlhe_state::call, py::return_value_policy::reference)
        .def("bet", &nlhe_state::raise, py::return_value_policy::reference)
        .def_property_readonly("pot", &nlhe_state::get_pot)
        .def_property_readonly("stack_size", &nlhe_state::get_stack_size)
        .def("__repr__", &nlhe_state::to_string)
        ;

    py::enum_<nlhe_state::game_round>(py_nlhe_state, "GameRound", py::arithmetic())
        .value("INVALID_ROUND", nlhe_state::INVALID_ROUND)
        .value("PREFLOP", nlhe_state::PREFLOP)
        .value("FLOP", nlhe_state::FLOP)
        .value("TURN", nlhe_state::TURN)
        .value("RIVER", nlhe_state::RIVER)
        .value("ROUNDS", nlhe_state::ROUNDS)
        .export_values();

    py::enum_<nlhe_state::holdem_action>(py_nlhe_state, "HoldemAction", py::arithmetic())
        .value("INVALID_ACTION", nlhe_state::INVALID_ACTION)
        .value("FOLD", nlhe_state::FOLD)
        .value("CALL", nlhe_state::CALL)
        .value("RAISE_O", nlhe_state::RAISE_O)
        .value("RAISE_H", nlhe_state::RAISE_H)
        .value("RAISE_Q", nlhe_state::RAISE_Q)
        .value("RAISE_P", nlhe_state::RAISE_P)
        .value("RAISE_W", nlhe_state::RAISE_W)
        .value("RAISE_D", nlhe_state::RAISE_D)
        .value("RAISE_V", nlhe_state::RAISE_V)
        .value("RAISE_T", nlhe_state::RAISE_T)
        .value("RAISE_A", nlhe_state::RAISE_A)
        .value("ACTIONS", nlhe_state::ACTIONS)
        .export_values();

    return m.ptr();
}
