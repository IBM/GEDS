/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef UTILITY_PATH_H
#define UTILITY_PATH_H

/**
 * A path class that implements heteregnous lookup for prefix searches.
 * https://www.cppstories.com/2019/05/heterogeneous-lookup-cpp14/
 * See also this SE discussion: https://stackoverflow.com/a/44723860/592024
 */

#include <string>
#include <string_view>
#include <utility>

namespace utility {

struct PathPrefixProbe;
struct Path {
  std::string name;

  bool operator<(const std::string &other);
  bool operator<(const std::string_view &other);
  bool operator<(const Path &other) const { return name < other.name; }
  bool operator<(const PathPrefixProbe &probe) const;
};

struct PathPrefixProbe {
  std::string_view prefix;
  bool operator<(const Path &path) const;
};

template <typename T> auto prefixSearch(const T &container, const std::string_view &prefix) {
  return container.equal_range(utility::PathPrefixProbe{prefix});
}

} // namespace utility

#endif
