/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include "GEDSProtocol.h"

#include <magic_enum.hpp>

std::string geds::to_string(geds::Protocol protocol) {
  return std::string{magic_enum::enum_name(protocol)};
}
