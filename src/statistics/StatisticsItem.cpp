/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StatisticsItem.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

static std::string createPrometheusLabel(std::string label) {
  boost::replace_all(label, ":", "_");
  boost::replace_all(label, " ", "_");
  boost::to_lower(label);
  return label;
}

namespace geds {
StatisticsItem::StatisticsItem(std::string labelArg)
    : label(std::move(labelArg)), prometheusLabel(createPrometheusLabel(label)) {}

} // namespace geds
