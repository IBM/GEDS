/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sstream>
#include <string>

namespace geds {

class StatisticsItem {
public:
  StatisticsItem(std::string label);
  virtual ~StatisticsItem() = default;
  const std::string label;
  const std::string prometheusLabel;

  virtual void printForPrometheus(std::stringstream &stream) const = 0;
  virtual void printForConsole(std::stringstream &stream) const = 0;

  StatisticsItem virtual &operator+=(size_t value) = 0;
};

} // namespace geds
