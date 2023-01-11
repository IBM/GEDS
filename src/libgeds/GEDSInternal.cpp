/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include "GEDSInternal.h"

#include <magic_enum.hpp>

namespace geds {

std::string to_string(ConnectionState state) { return std::string{magic_enum::enum_name(state)}; }

std::string to_string(ServiceState state) { return std::string{magic_enum::enum_name(state)}; }

} // namespace geds
