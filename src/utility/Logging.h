/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <chrono>
#include <iostream>

#include "LogNullSink.h"

namespace geds::logging {

template <typename T, typename... Msg> inline void LogLine(T &dest, const Msg &...msg) {
  (dest << ... << msg) << std::endl;
}

template <typename T, typename... Msg> inline void LogTimestamp(T &dest, const Msg &...msg) {
  dest << std::chrono::system_clock::now() << " - ";
  (dest << ... << msg) << std::endl;
}

} // namespace geds::logging

#define LOG_LINE __func__, " (", __FILE__, ": ", __LINE__, "): "                          // NOLINT
#define LOG_ERROR(...) geds::logging::LogLine(std::cerr, "ERROR ", LOG_LINE, __VA_ARGS__) // NOLINT
#define LOG_INFO(...) geds::logging::LogLine(std::clog, "INFO ", LOG_LINE, __VA_ARGS__)   // NOLINT
#define LOG_WARNING(...)                                                                           \
  geds::logging::LogLine(std::clog, "WARN  ", LOG_LINE, __VA_ARGS__) // NOLINT

#if defined(NDEBUG)
#define LOG_DEBUG(...)                                                                             \
  geds::logging::LogLine(::utility::NULL_SINK, "DEBUG ", LOG_LINE, __VA_ARGS__) // NOLINT
#else
#define LOG_DEBUG(...) geds::logging::LogLine(std::clog, "DEBUG ", LOG_LINE, __VA_ARGS__) // NOLINT
#endif
