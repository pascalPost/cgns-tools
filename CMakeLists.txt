# Copyright (c) 2022 Pascal Post
# This code is licensed under MIT license (see LICENSE.txt for details)

cmake_minimum_required(VERSION 3.1.0)
project(cgns_struct2unstruct VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(lib)
add_subdirectory(cli)
add_subdirectory(pythonInterface)