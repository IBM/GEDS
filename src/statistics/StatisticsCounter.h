/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

#include "StatisticsItem.h"

namespace geds {

class StatisticsCounter : public StatisticsItem {
  mutable std::atomic<size_t> _count;
  StatisticsCounter(std::string labelArg);

public:
  ~StatisticsCounter() override = default;
  void printForPrometheus(std::stringstream &stream) const override;
  void printForConsole(std::stringstream &stream) const override;

  StatisticsCounter &operator+=(size_t value) override;

  static std::shared_ptr<StatisticsCounter> factory(std::string labelArg);
};
} // namespace geds
