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

namespace geds {

Statistics &Statistics::get() {
  static Statistics instance;
  return instance;
}

void Statistics::print() {
  auto &instance = Statistics::get();
  instance.printStatistics();
}

std::shared_ptr<StatisticsCounter> Statistics::counter(const std::string &label) {
  auto &instance = Statistics::get();
  return instance.getCounter(label);
}

std::shared_ptr<StatisticsCounter> Statistics::getCounter(const std::string &label) {
  auto exists = _statistics.get(label);
  if (exists.has_value()) {
    return *exists;
  }
  auto newValue = StatisticsCounter::factory(label);
  return _statistics.insertOrExists(label, newValue);
}

void Statistics::printStatistics() const {
  std::stringstream msg;
  msg << "GEDS Statistics:" << std::endl;
  msg << "Name, Value" << std::endl;
  auto printFunction = [&msg](const std::string &k, std::shared_ptr<StatisticsCounter> c) {
    msg << k << ", " << std::to_string(c->get()) << std::endl;
  };
  _statistics.run(printFunction);
  msg << std::endl;
  LOG_INFO(msg.str());
}

void Statistics::prometheusMetrics(std::stringstream &stream) const {
  _statistics.run([&stream](const std::string &, std::shared_ptr<StatisticsCounter> &counter) {
    stream << "# TYPE " << counter->prometheusLabel << " counter"
           << "\n" //
           << counter->prometheusLabel << " " << counter->get() << "\n";
  });
}

} // namespace geds
