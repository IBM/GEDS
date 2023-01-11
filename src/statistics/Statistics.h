/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#pragma once

#include <memory>

#include "ConcurrentMap.h"
#include "StatisticsCounter.h"

namespace geds {
class Statistics {
  utility::ConcurrentMap<std::string, std::shared_ptr<StatisticsCounter>> _statistics;
  Statistics() = default;

public:
  ~Statistics() = default;
  Statistics(const Statistics &) = delete;
  Statistics(Statistics &&) = delete;
  Statistics &operator=(const Statistics &) = delete;
  Statistics &operator=(Statistics &&) = delete;

  static Statistics &get();
  static void print();
  static std::shared_ptr<StatisticsCounter> counter(const std::string &label);

  std::shared_ptr<StatisticsCounter> getCounter(const std::string &label);
  void printStatistics() const;
};
} // namespace geds
