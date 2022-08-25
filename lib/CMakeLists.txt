# Copyright (c) 2022 Pascal Post
# This code is licensed under MIT license (see LICENSE.txt for details)

add_library(cgns-tools SHARED src/cgns-tools.cpp)

find_package(CGNS REQUIRED)
target_link_libraries(cgns-tools CGNS::CGNS)

# for comfortable import within other cmake projects
target_include_directories(cgns-tools PUBLIC include)

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
