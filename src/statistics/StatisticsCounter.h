/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

namespace geds {

class StatisticsCounter {
  mutable std::atomic<size_t> _count;
  StatisticsCounter(std::string labelArg = "");

public:
  const std::string label;
  void increase(size_t s = 1);
  [[nodiscard]] size_t get() const;

  [[nodiscard]] std::string getAsString() const;

  StatisticsCounter &operator+=(size_t value);
  StatisticsCounter &operator++();

  static std::shared_ptr<StatisticsCounter> factory(std::string labelArg = "");
};
} // namespace geds
