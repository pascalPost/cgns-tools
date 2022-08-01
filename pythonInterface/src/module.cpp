// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#include "../../include/cgns.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(cgns_struct2unstruct_pySDK, m) {
  m.doc() = "cgns_struct2unstruct python interface module";

  // py::class_<cgns::fileIn>(m, "cgnsFile")
  //     .def(py::init<const std::string &>())
  //     .def("baseInformation", &cgns::file::readBaseInformation);

  // m.def("add", &add, "A function that adds two numbers");
}
