/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "theta_sketch.hpp"
#include "theta_union.hpp"
#include "theta_intersection.hpp"
#include "theta_a_not_b.hpp"
#include "theta_jaccard_similarity.hpp"
#include "common_defs.hpp"

namespace py = pybind11;

void init_theta(py::module &m) {
  using namespace datasketches;

  py::class_<theta_sketch>(m, "theta_sketch")
    .def("__str__", &theta_sketch::to_string, py::arg("print_items")=false,
         "Produces a string summary of the sketch")
    .def("to_string", &theta_sketch::to_string, py::arg("print_items")=false,
         "Produces a string summary of the sketch")
    .def("is_empty", &theta_sketch::is_empty,
         "Returns True if the sketch is empty, otherwise False")
    .def("get_estimate", &theta_sketch::get_estimate,
         "Estimate of the distinct count of the input stream")
    .def("get_upper_bound", &theta_sketch::get_upper_bound, py::arg("num_std_devs"),
         "Returns an approximate upper bound on the estimate at standard deviations in {1, 2, 3}")
    .def("get_lower_bound", &theta_sketch::get_lower_bound, py::arg("num_std_devs"),
         "Returns an approximate lower bound on the estimate at standard deviations in {1, 2, 3}")
    .def("is_estimation_mode", &theta_sketch::is_estimation_mode,
         "Returns True if sketch is in estimation mode, otherwise False")
    .def("get_theta", &theta_sketch::get_theta,
         "Returns theta (effective sampling rate) as a fraction from 0 to 1")
    .def("get_theta64", &theta_sketch::get_theta64,
         "Returns theta as 64-bit value")
    .def("get_num_retained", &theta_sketch::get_num_retained,
         "Returns the number of items currently in the sketch")
    .def("get_seed_hash", &theta_sketch::get_seed_hash,
         "Returns a hash of the seed used in the sketch")
    .def("is_ordered", &theta_sketch::is_ordered,
         "Returns True if the sketch entries are sorted, otherwise False")
    .def("__iter__", [](const theta_sketch& s) { return py::make_iterator(s.begin(), s.end()); })
  ;

  py::class_<update_theta_sketch, theta_sketch>(m, "update_theta_sketch")
    .def(
        py::init([](uint8_t lg_k, double p, uint64_t seed) {
          return update_theta_sketch::builder().set_lg_k(lg_k).set_p(p).set_seed(seed).build();
        }),
        py::arg("lg_k")=theta_constants::DEFAULT_LG_K, py::arg("p")=1.0, py::arg("seed")=DEFAULT_SEED
    )
    .def(py::init<const update_theta_sketch&>())
    .def("update", (void (update_theta_sketch::*)(int64_t)) &update_theta_sketch::update, py::arg("datum"),
         "Updates the sketch with the given integral value")
    .def("update", (void (update_theta_sketch::*)(double)) &update_theta_sketch::update, py::arg("datum"),
         "Updates the sketch with the given floating point value")
    .def("update", (void (update_theta_sketch::*)(const std::string&)) &update_theta_sketch::update, py::arg("datum"),
         "Updates the sketch with the given string")
    .def("compact", &update_theta_sketch::compact, py::arg("ordered")=true,
         "Returns a compacted form of the sketch, optionally sorting it")
    .def("trim", &update_theta_sketch::trim, "Removes retained entries in excess of the nominal size k (if any)")
    .def("reset", &update_theta_sketch::reset, "Resets the sketch to the initial empty state")
  ;

  py::class_<compact_theta_sketch, theta_sketch>(m, "compact_theta_sketch")
    .def(py::init<const compact_theta_sketch&>())
    .def(py::init<const theta_sketch&, bool>())
    .def(
        "serialize",
        [](const compact_theta_sketch& sk) {
          auto bytes = sk.serialize();
          return py::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        },
        "Serializes the sketch into a bytes object"
    )
    .def_static(
        "deserialize",
        [](const std::string& bytes, uint64_t seed) {
          return compact_theta_sketch::deserialize(bytes.data(), bytes.size(), seed);
        },
        py::arg("bytes"), py::arg("seed")=DEFAULT_SEED,
        "Reads a bytes object and returns the corresponding compact_theta_sketch"
    );

  py::class_<theta_union>(m, "theta_union")
    .def(
        py::init([](uint8_t lg_k, double p, uint64_t seed) {
          return theta_union::builder().set_lg_k(lg_k).set_p(p).set_seed(seed).build();
        }),
        py::arg("lg_k")=theta_constants::DEFAULT_LG_K, py::arg("p")=1.0, py::arg("seed")=DEFAULT_SEED
    )
    .def("update", &theta_union::update<const theta_sketch&>, py::arg("sketch"),
         "Updates the union with the given sketch")
    .def("get_result", &theta_union::get_result, py::arg("ordered")=true,
         "Returns the sketch corresponding to the union result")
  ;

  py::class_<theta_intersection>(m, "theta_intersection")
    .def(py::init<uint64_t>(), py::arg("seed")=DEFAULT_SEED)
    .def(py::init<const theta_intersection&>())
    .def("update", &theta_intersection::update<const theta_sketch&>, py::arg("sketch"),
         "Intersections the provided sketch with the current intersection state")
    .def("get_result", &theta_intersection::get_result, py::arg("ordered")=true,
         "Returns the sketch corresponding to the intersection result")
    .def("has_result", &theta_intersection::has_result,
         "Returns True if the intersection has a valid result, otherwise False")
  ;

  py::class_<theta_a_not_b>(m, "theta_a_not_b")
    .def(py::init<uint64_t>(), py::arg("seed")=DEFAULT_SEED)
    .def(
        "compute",
        &theta_a_not_b::compute<const theta_sketch&, const theta_sketch&>,
        py::arg("a"), py::arg("b"), py::arg("ordered")=true,
        "Returns a sketch with the result of applying the A-not-B operation on the given inputs"
    )
  ;
  
  py::class_<theta_jaccard_similarity>(m, "theta_jaccard_similarity")
    .def_static(
        "jaccard",
        [](const theta_sketch& sketch_a, const theta_sketch& sketch_b, uint64_t seed) {
          return theta_jaccard_similarity::jaccard(sketch_a, sketch_b, seed);
        },
        py::arg("sketch_a"), py::arg("sketch_b"), py::arg("seed")=DEFAULT_SEED,
        "Returns a list with {lower_bound, estimate, upper_bound} of the Jaccard similarity between sketches"
    )
    .def_static(
        "exactly_equal",
        &theta_jaccard_similarity::exactly_equal<const theta_sketch&, const theta_sketch&>,
        py::arg("sketch_a"), py::arg("sketch_b"), py::arg("seed")=DEFAULT_SEED,
        "Returns True if sketch_a and sketch_b are equivalent, otherwise False"
    )
    .def_static(
        "similarity_test",
        &theta_jaccard_similarity::similarity_test<const theta_sketch&, const theta_sketch&>,
        py::arg("actual"), py::arg("expected"), py::arg("threshold"), py::arg("seed")=DEFAULT_SEED,
        "Tests similarity of an actual sketch against an expected sketch. Computers the lower bound of the Jaccard "
        "index J_{LB} of the actual and expected sketches. If J_{LB} >= threshold, then the sketches are considered "
        "to be similar with a confidence of 97.7% and returns True, otherwise False.")
    .def_static(
        "dissimilarity_test",
        &theta_jaccard_similarity::dissimilarity_test<const theta_sketch&, const theta_sketch&>,
        py::arg("actual"), py::arg("expected"), py::arg("threshold"), py::arg("seed")=DEFAULT_SEED,
        "Tests dissimilarity of an actual sketch against an expected sketch. Computers the lower bound of the Jaccard "
        "index J_{UB} of the actual and expected sketches. If J_{UB} <= threshold, then the sketches are considered "
        "to be dissimilar with a confidence of 97.7% and returns True, otherwise False."
    )
  ;     
}
