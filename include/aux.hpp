// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#pragma once

#include <type_traits>

namespace cgns {

template <typename T> struct always_false : std::false_type {};

} // namespace cgns