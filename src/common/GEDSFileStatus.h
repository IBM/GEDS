/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#pragma once

#include <string>

struct GEDSFileStatus {
  std::string key;
  size_t size;
  bool isDirectory;

  bool operator<(const GEDSFileStatus &other) const {
    if (isDirectory > other.isDirectory) {
      return true;
    }
    return key < other.key;
  }
  bool operator==(const GEDSFileStatus &other) const {
    return key == other.key && isDirectory == other.isDirectory;
  }
};
