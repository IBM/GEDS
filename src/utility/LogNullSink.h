/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include <iostream>
#include <ostream>

#ifndef UTILITY_LOG_NULL_SINK_H
#define UTILITY_LOG_NULL_SINK_H

namespace utility {
class LogNullSink : public std::ostream {
public:
  LogNullSink() : std::ostream(nullptr) {}
  LogNullSink(const LogNullSink &) : std::basic_ios<char>(), std::ostream(nullptr) {}
};

template <class T> const LogNullSink &operator<<(LogNullSink &&os, const T & /* unused value */) {
  return os;
}

extern LogNullSink NULL_SINK;
} // namespace utility
#endif
