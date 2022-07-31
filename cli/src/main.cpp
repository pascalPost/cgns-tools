// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#include "../../include/cgns.hpp"
#include "../../include/logger.hpp"
#include <iostream>

int main(int argc, char *argv[]) {

  spdlog::set_level(spdlog::level::warn);
  spdlog::cfg::load_argv_levels(
      argc, argv); // set log levels from argv, e.g. SPDLOG_LEVEL=info

  auto root =
      cgns::parse("/home/pascal/workspace/cgns_struct2unstruct/test_new.cgns");

  cgns::writeFile("/home/pascal/workspace/cgns_struct2unstruct/test_out.cgns",
                  root);
}
