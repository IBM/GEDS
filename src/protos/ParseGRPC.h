/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_PARSE_GRPC_H
#define GEDS_PARSE_GRPC_H

#include <string>

#include <absl/status/statusor.h>

namespace geds {
/**
 * @brief Extract the address from the grpc peer information.
 * The peer information has the format:
 * - IPv4: `ipv4:address[:port][,address[:port],...]`
 * - IPv6: `ipv6:address[:port][,address[:port],...]`
 * Precondition: context->peer only contains one address - the function will fail if multiple
 * addresses are provided. Note: DNS entries are silently accepted and the format is not validated.
 * See also: https://github.com/grpc/grpc/blob/master/doc/naming.md
 */
absl::StatusOr<std::string> GetAddressFromGRPCPeer(const std::string &peerInformation);
} // namespace geds

#endif
