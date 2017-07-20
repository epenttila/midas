#include <pybind11/pybind11.h>
#include "version.h"

namespace py = pybind11;

PYBIND11_PLUGIN(pyutil)
{
    py::module m("pyutil", "midas Python plugin");

    m.attr("GIT_VERSION") = util::GIT_VERSION;

    return m.ptr();
}
