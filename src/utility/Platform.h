/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef UTILITY_PLATFORM_H
#define UTILITY_PLATFORM_H

#include <cstdint>
#if __cplusplus >= 202300L
#include <utility>
#endif

#include <string>

namespace utility::platform {

// Unreachable
[[noreturn]] inline void unreachable() {
#if __cplusplus >= 202300L
  std::unreachable();
#else
#ifdef __GNUC__         // GCC, Clang, ICC
  __builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
  __assume(false);
#else
  // Unsupported platform.
  static_assert(false);
#endif
#endif
}

std::string getHostName();

} // namespace utility::platform
#endif // GEDS_GEDSPLATFORM_H
