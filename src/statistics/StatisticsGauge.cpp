/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StatisticsGauge.h"
#include <memory>

namespace geds {
StatisticsGauge::StatisticsGauge(std::string labelArg) : StatisticsItem(std::move(labelArg)) {}

void StatisticsGauge::printForPrometheus(std::stringstream &stream) const {
  stream << "# TYPE " << prometheusLabel << " gauge\n" << prometheusLabel << " " << _value << "\n";
}

void StatisticsGauge::printForConsole(std::stringstream &stream) const {
  stream << label << ", " << _value << std::endl;
}

StatisticsGauge &StatisticsGauge::operator=(size_t v) {
  _value = v;
  return *this;
}

StatisticsGauge &StatisticsGauge::operator+=(size_t v) {
  _value += v;
  return *this;
}

StatisticsGauge &StatisticsGauge::operator-=(size_t v) {
  _value -= v;
  return *this;
}

std::shared_ptr<StatisticsGauge> StatisticsGauge::factory(std::string labelArg) {
  return std::shared_ptr<StatisticsGauge>(new StatisticsGauge(std::move(labelArg)));
}
} // namespace geds
