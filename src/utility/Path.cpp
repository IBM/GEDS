/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Path.h"

#include <string_view>

namespace utility {

bool Path::operator<(const std::string &other) { return name < other; }

bool Path::operator<(const std::string_view &other) { return name < other; }

bool Path::operator<(const PathPrefixProbe &probe) const {
  std::string_view view = name;
  return view.substr(0, probe.prefix.size()) < probe.prefix;
}
bool PathPrefixProbe::operator<(const Path &path) const {
  std::string_view view = path.name;
  return prefix < view.substr(0, prefix.size());
}

} // namespace utility
