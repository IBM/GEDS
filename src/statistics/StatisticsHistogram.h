/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "StatisticsItem.h"

namespace geds {

class StatisticsHistogram : public StatisticsItem {
  std::atomic<size_t> _sum;
  std::atomic<size_t> _total;
  std::vector<std::atomic<size_t>> _count;
  const std::vector<size_t> &_buckets;
  StatisticsHistogram(std::string labelArg, const std::vector<size_t> &buckets)
      : StatisticsItem(std::move(labelArg)), _count(buckets.size() + 1), _buckets(buckets) {
    for (auto &c : _count) {
      c = (size_t)0;
    }
  }

public:
  ~StatisticsHistogram() override = default;
  void printForPrometheus(std::stringstream &stream) const override {
    stream << "# TYPE " << prometheusLabel << " histogram\n";
    for (size_t i = 0; i < _buckets.size(); i++) {
      stream << prometheusLabel << "_bucket{le=\"" << _buckets[i] << "\"} " << _count[i] << "\n";
    }
    stream << prometheusLabel << "_bucket{le=\"+Inf\"}" << _count[_buckets.size()] << "\n"
           << prometheusLabel << "_sum " << _sum << "\n"
           << prometheusLabel << "_count " << _total << std::endl;
  }

  void printForConsole(std::stringstream &stream) const override {
    stream << label << ", " << _total << ", sum " << _sum << ", ";
    for (size_t i = 0; i < _buckets.size(); i++) {
      stream << " {le=" << _buckets[i] << "} " << _count[i] << ", ";
    }
    stream << " {le=+Inf }" << _count[_buckets.size()] << std::endl;
  }

  StatisticsItem &operator+=(size_t value) override {
    _sum += value;
    _total += 1;
    for (size_t i = 0; i < _buckets.size(); i++) {
      if (value < _buckets[i]) {
        return *this;
      }
      _count[i] += 1;
    }
    _count[_buckets.size()] += 1;
    return *this;
  }

  static std::shared_ptr<StatisticsHistogram> factory(std::string labelArg,
                                                      const std::vector<size_t> &buckets) {
    return std::shared_ptr<StatisticsHistogram>(
        new StatisticsHistogram(std::move(labelArg), std::move(buckets)));
  }
};
} // namespace geds
