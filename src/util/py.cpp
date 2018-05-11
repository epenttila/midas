#include <pybind11/pybind11.h>
#include "version.h"
#include "metric.h"

namespace py = pybind11;

PYBIND11_PLUGIN(pyutil)
{
    py::module m("pyutil", "midas Python plugin");

    m.attr("GIT_VERSION") = util::GIT_VERSION;
    m.def("get_hamming_distance", &get_hamming_distance);

    return m.ptr();
}
