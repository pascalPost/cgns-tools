// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#pragma once

#include "spdlog/cfg/argv.h"
#include "spdlog/fmt/bundled/format.h"
#include <spdlog/spdlog.h>

#include <string_view>

template <typename... Args>
std::string indent(const unsigned indent, std::string_view format_str,
                   Args &&...args) {
  return fmt::format("{:{}}", "", indent)
      .append(fmt::format(format_str, std::forward<Args>(args)...));
}