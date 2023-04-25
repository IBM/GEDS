/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "StatisticsItem.h"

namespace geds {

class StatisticsGauge : public StatisticsItem {

  std::atomic<int64_t> _value;
  StatisticsGauge(std::string labelArg);

public:
  ~StatisticsGauge() override = default;

  void printForPrometheus(std::stringstream &stream) const override;
  void printForConsole(std::stringstream &stream) const override;

  StatisticsGauge &operator=(size_t v);
  StatisticsGauge &operator+=(size_t v) override;
  StatisticsGauge &operator-=(size_t v) override;

  static std::shared_ptr<StatisticsGauge> factory(std::string labelArg);
};

} // namespace geds
