/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_GEDSPROTOCOL_H
#define GEDS_GEDSPROTOCOL_H

#include <string>

namespace geds {
enum class Protocol { socket, rdma, s3a, cos };
std::string to_string(geds::Protocol protocol);

} // namespace geds

#endif // GEDS_GEDSPROTOCOL_H
