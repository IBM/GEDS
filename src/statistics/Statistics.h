/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include "ConcurrentMap.h"
#include "StatisticsCounter.h"
#include "StatisticsHistogram.h"
#include "StatisticsItem.h"

namespace geds {
class Statistics {
  utility::ConcurrentMap<std::string, std::shared_ptr<StatisticsCounter>> _counters;
  utility::ConcurrentMap<std::string, std::shared_ptr<StatisticsHistogram>> _histograms;

  Statistics() = default;

public:
  ~Statistics() = default;
  Statistics(const Statistics &) = delete;
  Statistics(Statistics &&) = delete;
  Statistics &operator=(const Statistics &) = delete;
  Statistics &operator=(Statistics &&) = delete;

  static Statistics &get();
  static void print();
  static std::shared_ptr<StatisticsCounter> createCounter(const std::string &label);

  static std::shared_ptr<StatisticsHistogram> createHistogram(const std::string &label,
                                                              const std::vector<size_t> &buckets);
  static std::shared_ptr<StatisticsHistogram> createIOHistogram(const std::string &label);
  static std::shared_ptr<StatisticsHistogram> createNanoSecondHistogram(const std::string &label);

  void printStatistics() const;
  void prometheusMetrics(std::stringstream &stream) const;
};
} // namespace geds
