/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

template <typename Clock, typename Duration>
std::ostream &operator<<(std::ostream &stream,
                         const std::chrono::time_point<Clock, Duration> &time) {
  const auto t = Clock::to_time_t(time);
  // Print timestamps in ISO8601 standard.
  return stream << std::put_time(std::gmtime(&t), "%Y-%m-%dT%TZ");
}

template <typename Clock, typename Duration>
std::string toISO8601String(const std::chrono::time_point<Clock, Duration> &time) {
  std::stringstream ss;
  ss << time;
  return ss.str();
}
