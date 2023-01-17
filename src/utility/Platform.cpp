/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Platform.h"

#include <climits>
#include <stdexcept>
#include <unistd.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

namespace utility {
namespace platform {

std::string getHostName() {
  char hostname[std::max(HOST_NAME_MAX, 255) + 1] = {0};
  int result = gethostname(hostname, std::size(hostname));
  if (result != 0) {
    throw std::runtime_error("Unable to execute gethostname!");
  }
  return std::string(hostname);
}

} // namespace platform
} // namespace utility
