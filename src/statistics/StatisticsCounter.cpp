/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StatisticsCounter.h"
#include "StatisticsItem.h"
#include <memory>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace geds {
StatisticsCounter::StatisticsCounter(std::string labelArg) : StatisticsItem(std::move(labelArg)) {}

void StatisticsCounter::printForPrometheus(std::stringstream &stream) const {
  stream << "# TYPE " << prometheusLabel << " counter"
         << "\n" //
         << prometheusLabel << " " << _count << "\n";
}
void StatisticsCounter::printForConsole(std::stringstream &stream) const {
  stream << label << ", " << _count << "\n";
}

std::shared_ptr<StatisticsCounter> StatisticsCounter::factory(std::string labelArg) {
  return std::shared_ptr<StatisticsCounter>(new StatisticsCounter(std::move(labelArg)));
}

StatisticsCounter &StatisticsCounter::operator+=(size_t value) {
  _count += value;
  return *this;
}

} // namespace geds
