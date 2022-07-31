# Copyright (c) 2022 Pascal Post
# This code is licensed under MIT license (see LICENSE.txt for details)

# pybind11, see https://github.com/smrfeld/cmake_cpp_pybind11_tutorial

include(FetchContent)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.10.0
    )

FetchContent_GetProperties(pybind11)

if(NOT pybind11_POPULATED)
    FetchContent_Populate(pybind11)
    add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
endif()

pybind11_add_module(cgns_struct2unstruct_pySDK src/module.cpp)
target_link_libraries(cgns_struct2unstruct_pySDK PRIVATE cgns_struct2unstruct)