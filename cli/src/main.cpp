// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#include <cgns-tools.hpp>
#include <iostream>
#include <logger.hpp>

int main(int argc, char *argv[]) {

  spdlog::set_level(spdlog::level::warn);
  spdlog::cfg::load_argv_levels(
      argc, argv); // set log levels from argv, e.g. SPDLOG_LEVEL=info

  auto root = cgns_tools::parse(
      "/home/pascal/workspace/cgns_struct2unstruct/test_new.cgns");

  cgns_tools::writeFile(
      "/home/pascal/workspace/cgns_struct2unstruct/test_out.cgns", root);
}
