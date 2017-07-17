#include <pybind11/pybind11.h>
#include "nlhe_strategy.h"

namespace py = pybind11;

PYBIND11_PLUGIN(pycfrlib)
{
    py::module m("pycfrlib", "midas Python plugin");

    py::class_<nlhe_strategy>(m, "NLHEStrategy")
        .def(py::init<const std::string&, bool>())
        .def_property_readonly("abstraction", &nlhe_strategy::get_abstraction, py::return_value_policy::reference)
        .def_property_readonly("root_state", &nlhe_strategy::get_root_state, py::return_value_policy::reference)
        .def_property_readonly("strategy", py::overload_cast<>(&nlhe_strategy::get_strategy), py::return_value_policy::reference)
        .def_property_readonly("stack_size", &nlhe_strategy::get_stack_size)
        ;

    py::class_<strategy>(m, "Strategy")
        .def("get_probability", &strategy::get_probability)
        .def("get_random_child", &strategy::get_random_child, py::return_value_policy::reference)
        .def_property_readonly("filename", &strategy::get_filename)
        ;

    return m.ptr();
}
