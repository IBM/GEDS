/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "GEDSConfig.h"

struct GEDSConfigContainer {
  std::shared_ptr<GEDSConfig> element;
};
