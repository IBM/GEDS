/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Statistics.h"

#include <memory>
#include <sstream>
#include <string>

#include "Logging.h"
#include "StatisticsCounter.h"
#include "StatisticsGauge.h"
#include "StatisticsHistogram.h"

namespace geds {

Statistics &Statistics::get() {
  static Statistics instance;
  return instance;
}

void Statistics::print() {
  auto &instance = Statistics::get();
  instance.printStatistics();
}

std::shared_ptr<StatisticsCounter> Statistics::createCounter(const std::string &label) {
  auto &instance = Statistics::get();
  auto value = instance._counters.get(label);
  if (value.has_value()) {
    return *value;
  }
  auto item = StatisticsCounter::factory(label);
  return instance._counters.insertOrExists(label, item);
}

std::shared_ptr<StatisticsGauge> Statistics::createGauge(const std::string &label) {
  auto &instance = Statistics::get();
  auto value = instance._gauges.get(label);
  if (value.has_value()) {
    return *value;
  }
  auto item = StatisticsGauge::factory(label);
  return instance._gauges.insertOrExists(label, item);
}

std::shared_ptr<StatisticsHistogram>
Statistics::createHistogram(const std::string &label, const std::vector<size_t> &buckets) {
  auto &instance = Statistics::get();
  auto value = instance._histograms.get(label);
  if (value.has_value()) {
    return *value;
  }
  auto item = StatisticsHistogram::factory(label, buckets);
  return instance._histograms.insertOrExists(label, item);
}

std::shared_ptr<StatisticsHistogram> Statistics::createIOHistogram(const std::string &label) {
  static auto vec = [] {
    std::vector<size_t> v(24);
    for (size_t i = 0; i < v.size(); i++) {
      v[i] = 1 << (i + 6); // (1<<6) = 128
    }
    return v;
  }();
  return createHistogram(label, vec);
}

std::shared_ptr<StatisticsHistogram>
Statistics::createNanoSecondHistogram(const std::string &label) {
  constexpr size_t ONE_MIN = 60000000000; // Minute
  const static auto NANOSECOND_HISTOGRAM = std::vector<size_t>{
      1,        10,        100,        1000,        10000,       100000,       1000000,
      10000000, 100000000, 1000000000, 10000000000, ONE_MIN * 1, ONE_MIN * 10, ONE_MIN * 60};

  return createHistogram(label, NANOSECOND_HISTOGRAM);
}

void Statistics::printStatistics() const {
  std::stringstream msg;
  msg << "GEDS Statistics:" << std::endl;
  msg << "Name, Value" << std::endl;
  _counters.forall([&msg](auto &c) { c->printForConsole(msg); });
  _gauges.forall([&msg](auto &c) { c->printForConsole(msg); });
  _histograms.forall([&msg](auto &c) { c->printForConsole(msg); });
  msg << std::endl;
  LOG_INFO(msg.str());
}

void Statistics::prometheusMetrics(std::stringstream &stream) const {
  _counters.forall(
      [&stream](const std::string &, auto &item) { item->printForPrometheus(stream); });
  _gauges.forall([&stream](const std::string &, auto &item) { item->printForPrometheus(stream); });
  _histograms.forall(
      [&stream](const std::string &, auto &item) { item->printForPrometheus(stream); });
}

} // namespace geds
