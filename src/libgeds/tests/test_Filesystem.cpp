/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include "gtest/gtest.h"

#include <absl/strings/escaping.h>
#include <string>

#include <gtest/gtest.h>

TEST(Filesystem, Encoding) {
  const std::string encoded = absl::WebSafeBase64Escape("/");
  assert(encoded.find("/") == std::string::npos);
}
