# Copyright (c) 2022 Pascal Post
# This code is licensed under MIT license (see LICENSE.txt for details)

cmake_minimum_required(VERSION 3.1.0)
project(cgns_struct2unstruct VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


add_library(cgns-tools SHARED src/cgns.cpp)


find_package(CGNS REQUIRED)
target_link_libraries(cgns-tools CGNS::CGNS)


include(ExternalProject)

ExternalProject_Add(
    spdlog
    PREFIX spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.10.0
    CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
) 
ExternalProject_Get_property(spdlog SOURCE_DIR)
set(SPDLOG_INCLUDES "${SOURCE_DIR}/include")

add_dependencies(cgns-tools spdlog)
target_include_directories(cgns-tools PUBLIC ${SPDLOG_INCLUDES})


add_subdirectory(cli)
add_subdirectory(pythonInterface)