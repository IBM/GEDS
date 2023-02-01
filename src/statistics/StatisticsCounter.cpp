/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StatisticsCounter.h"
#include <memory>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

static std::string createPrometheusLabel(std::string label) {
  boost::replace_all(label, ":", "_");
  boost::replace_all(label, " ", "_");
  boost::to_lower(label);
  return label;
}

namespace geds {
StatisticsCounter::StatisticsCounter(std::string labelArg)
    : label(std::move(labelArg)), prometheusLabel(createPrometheusLabel(label)) {}

void StatisticsCounter::increase(size_t s) { _count += s; }

size_t StatisticsCounter::get() const { return _count.load(); }

std::string StatisticsCounter::getAsString() const { return label + " " + std::to_string(_count); }

std::shared_ptr<StatisticsCounter> StatisticsCounter::factory(std::string labelArg) {
  return std::shared_ptr<StatisticsCounter>(new StatisticsCounter(std::move(labelArg)));
}

StatisticsCounter &StatisticsCounter::operator+=(size_t value) {
  _count += value;
  return *this;
}
StatisticsCounter &StatisticsCounter::operator++() {
  ++_count;
  return *this;
}

} // namespace geds
