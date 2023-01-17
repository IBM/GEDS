/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ParseGRPC.h"
#include <string>

namespace geds {

absl::StatusOr<std::string> GetAddressFromGRPCPeer(const std::string &peerInfo) {
  const std::string_view ipv4Prefix{"ipv4:"};
  const std::string_view ipv6Prefix{"ipv6:"};

  if (peerInfo.find(',') != std::string::npos) {
    return absl::UnknownError("Peer information " + peerInfo + " contains multiple IP addresses!");
  }

  auto view = peerInfo.substr(ipv4Prefix.size());

  if (peerInfo.compare(0, ipv4Prefix.size(), ipv4Prefix) == 0) {
    // IPv4 (format: "ipv4:address[:port]")
    auto pos = view.find(':');
    if (pos == 0) {
      return absl::UnknownError("Unable to parse ip from peer " + peerInfo +
                                " (unexpected length)");
    }
    return std::string{view.substr(0, pos)};
  }
  if (peerInfo.compare(0, ipv6Prefix.size(), ipv6Prefix) == 0) {
    // IPv6 (format: `ipv6:address[:port]`)
    // Examples:  `ipv6:[2607:f8b0:400e:c00::ef]:443` or `ipv6:[::]:1234`
    auto pos = view.find(']');
    if (pos == 0) {
      return absl::UnknownError("Invalid IPv6 peer " + peerInfo);
    }
    if (pos != std::string::npos) {
      pos += 1;
    }
    return std::string{view.substr(0, pos)};
  }
  return absl::UnknownError("Unable to parse peer " + peerInfo + " (unknown GRPC peer format)");
}

} // namespace geds
