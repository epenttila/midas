#include <pybind11/pybind11.h>
#include "abslib/holdem_abstraction_base.h"

namespace py = pybind11;

PYBIND11_PLUGIN(pyabslib)
{
    py::module m("pyabslib", "midas Python plugin");

    py::class_<holdem_abstraction_base>(m, "HoldemAbstractionBase")
        .def("get_buckets", [](holdem_abstraction_base* p, int a, int b, int c, int d, int e, int f, int g)
            {
                holdem_abstraction_base::bucket_type buckets;
                p->get_buckets(a, b, c, d, e, f, g, &buckets);
                return std::make_tuple(buckets[0], buckets[1], buckets[2], buckets[3]);
            })
        .def("read", &holdem_abstraction_base::read)
        .def("get_bucket_count", &holdem_abstraction_base::get_bucket_count)
        ;

    return m.ptr();
}
