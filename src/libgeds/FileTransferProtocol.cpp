/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include "FileTransferProtocol.h"

#include "geds.pb.h"

static_assert(static_cast<int>(geds::rpc::FileTransferProtocol::Socket) ==
              static_cast<int>(geds::FileTransferProtocol::Socket));

static_assert(static_cast<int>(geds::rpc::FileTransferProtocol::RDMA) ==
              static_cast<int>(geds::FileTransferProtocol::RDMA));

namespace geds {

const std::vector<geds::rpc::FileTransferProtocol> &supportedProtocols() {
  static const auto result = std::vector<geds::rpc::FileTransferProtocol> {
    geds::rpc::FileTransferProtocol::Socket,
#if HAVE_RDMA
        FileTransferProtocol::RDMA,
#endif
  };
  return result;
}
} // namespace geds
