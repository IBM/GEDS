/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_FORMAT_TIME_ISO8601_H
#define GEDS_FORMAT_TIME_ISO8601_H

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>

template <typename Clock, typename Duration>
std::ostream &operator<<(std::ostream &stream,
                         const std::chrono::time_point<Clock, Duration> &time) {
  const auto t = Clock::to_time_t(time);
  // Print timestamps in ISO8601 standard.
  return stream << std::put_time(std::gmtime(&t), "%Y-%m-%dT%TZ");
}

#endif
