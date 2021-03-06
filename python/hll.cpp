#include "python/pysketch.h"

PYBIND11_MODULE(hll, m) {
    m.doc() = "HyperLogLog support"; // optional module docstring
    py::class_<hll_t> (m, "hll")
        .def(py::init<size_t>())
        .def(py::init<std::string>())
        .def("clear", &hll_t::clear, "Clear all entries.")
        .def("resize", &hll_t::resize, "Change old size to a new size.")
        .def("sum", &hll_t::sum, "Add up results.")
        .def("csum", &hll_t::csum, "Only add up results if no cached value is present.")
        .def("report", &hll_t::report, "Emit estimated cardinality. Performs sum if not performed, but sum must be recalculated if further entries are added.")
        .def("add", [](hll_t &h1, uint64_t v) {h1.add(v);}, "Add a (hashed) value to the sketch.")
        .def("addh_", [](hll_t &h1, uint64_t v) {h1.addh(h1.hash(v));}, "Hash an integer value and then add that to the sketch..")
        .def("jaccard_index", [](hll_t &h1, hll_t &h2) {return jaccard_index(h2, h2);})
        .def("sprintf", &hll_t::sprintf)
        .def("union", [](const hll_t &h1, const hll_t &h2) {return h1 + h2;})
        .def("union_size", [](const hll_t &h1, const hll_t &h2) {return h1.union_size(h2);})
        .def("est_err", &hll_t::est_err, "Estimate error")
        .def("relative_error", &hll_t::relative_error, "Expected error for sketch");
    m.def("jaccard_index", [](hll_t &h1, hll_t &h2) {
            return jaccard_index(h1, h2);
        }, "Calculates jaccard indexes between two sketches")
    .def("from_shs", [](const py::array_t<uint64_t> &input, size_t ss=10) {
         hll_t ret(ss);
         auto ptr = input.data();
         for(ssize_t i = 0; i < input.size();ret.add(ptr[i++]));
         return ret;
    }, py::return_value_policy::take_ownership, "Creates an HLL sketch from a numpy array of 64-bit hashes.")
    .def("from_np", [](const py::array_t<uint64_t> &input, size_t ss=10) {
         hll_t ret(ss);
         auto ptr = input.data();
         for(ssize_t i = 0; i < input.size();ret.addh(ptr[i++]));
         return ret;
     }, py::return_value_policy::take_ownership, "Creates an HLL sketch from a numpy array of (unhashed) 64-bit integers")
    .def("union_size", [](const hll_t &h1, const hll_t &h2) {return h1.union_size(h2);}, "Calculate union size")
    .def("jaccard_matrix", [](py::list l) {return CmpFunc::apply(l, JIF());}, py::return_value_policy::take_ownership,
         "Compare sketches in parallel. Input: list of sketches. Output: numpy array of n-choose-2 flat matrix of JIs, float")
    .def("intersection_matrix", [](py::list l) {return CmpFunc::apply(l, ISF());}, "Compare sketches in parallel. Input: list of sketches. Output: n-choose-2 flat matrix of intersection_size, float")
    .def("containment_matrix", [](py::list l) {return AsymmetricCmpFunc<hll_t>::apply(l, CSF());}, py::return_value_policy::take_ownership, "Compare sketches in parallel. Input: list of sketches. Output: n-choose-2 flat matrix of intersection_size, float")
    .def("union_size_matrix", [](py::list l) {return CmpFunc::apply(l, USF());},
         "Compare sketches in parallel. Input: list of sketches.")
    .def("symmetric_containment_matrix", [](py::list l) {return CmpFunc::apply(l, SCF());},
         "Compare sketches in parallel. Input: list of sketches.")
    .def("ij2ind", [](size_t i, size_t j, size_t n) {return i < j ? (((i) * (n * 2 - i - 1)) / 2 + j - (i + 1)): (((j) * (n * 2 - j - 1)) / 2 + i - (j + 1));})
    .def("randset", [](size_t i) {
        static wy::WyHash<uint64_t, 4> gen(1337);
        py::array_t<uint64_t> ret({i});
        auto ptr = static_cast<uint64_t *>(ret.request().ptr);
        for(size_t j = 0; j < i; ptr[j++] = gen());
        return ret;
    }, py::return_value_policy::take_ownership, "Generate a 1d random numpy array")
    .def("tri2full", [](py::array_t<float> arr) {
        size_t dim = flat2fullsz(arr.size());
        py::array_t<float> ret({dim, dim});
        auto retptr = static_cast<float *>(ret.request().ptr), aptr = static_cast<float *>(arr.request().ptr);
        for(size_t i = 0; i < dim; ++i) {
            for(size_t j = 0; j < i; ++j) {
                size_t ind = (((j) * (dim * 2 - j - 1)) / 2 + i - (j + 1));
                retptr[dim * i + j] = aptr[ind];
            }
            retptr[dim * i + i] = 1.; // Jaccard index is 1 for this case.
            for(size_t j = i + 1; j < dim; ++j) {
                size_t ind = ((i * (dim * 2 - i - 1)) / 2 + j - (i + 1));
                retptr[dim * i + j] = aptr[ind];
            }
        }
        return ret;
    }, py::return_value_policy::take_ownership);
} // pybind11 module
