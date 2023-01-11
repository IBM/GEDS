/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */

#include "Path.h"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

TEST(Utility, Path) {
  std::vector<std::string> testValues{"",   "A",  "AA", "AB", "AC", "B", "BA",
                                      "BB", "BC", "C",  "CA", "CB", "CC"};
  std::set<utility::Path, std::less<>> testSet;
  for (const auto &i : testValues) {
    testSet.emplace(utility::Path{i});
  }
  auto [prefixStart, prefixEnd] = testSet.equal_range(utility::PathPrefixProbe{"B"});
  std::vector<std::string> result;

  for (auto it = prefixStart; it != prefixEnd; it++) {
    result.push_back(it->name);
  }
  std::sort(result.begin(), result.end());

  std::vector<std::string> expected = {"B", "BA", "BB", "BC"};
  ASSERT_EQ(result, expected);
}

TEST(Utility, Path_PrefixSearch) {
  std::vector<std::string> testValues{"",   "A",  "AA", "AB", "AC", "B", "BA",
                                      "BB", "BC", "C",  "CA", "CB", "CC"};
  std::set<utility::Path, std::less<>> testSet;
  for (const auto &i : testValues) {
    testSet.emplace(utility::Path{i});
  }
  auto [prefixStart, prefixEnd] = utility::prefixSearch(testSet, "B");
  std::vector<std::string> result;

  for (auto it = prefixStart; it != prefixEnd; it++) {
    result.push_back(it->name);
  }
  std::sort(result.begin(), result.end());

  std::vector<std::string> expected = {"B", "BA", "BB", "BC"};
  ASSERT_EQ(result, expected);
}
