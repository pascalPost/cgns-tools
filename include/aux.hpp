// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#pragma once

#include <type_traits>

namespace cgns {

template<typename T>
struct always_false : std::false_type
{
};

/// @brief helper type for the visitor, see
/// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

} // namespace cgns