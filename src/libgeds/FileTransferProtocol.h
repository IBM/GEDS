/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_PROTOCOL_H
#define FILE_TRANSFER_PROTOCOL_H

#include <cstddef>
#include <cstdint>
#include <future>
#include <string>
#include <sys/socket.h>

#if HAVE_RDMA
#include <rdma/rdma_cma.h>
#endif

#include "geds.pb.h"

namespace geds {
/**
 * @brief FileTransferProtocol
 *
 * The types of this file need to match the types in geds.proto.
 */
enum class FileTransferProtocol : uint8_t { Socket = 0, RDMA = 1 };

struct ObjTransferEndpoint {
  std::string hostname;
  struct sockaddr laddr;
  struct sockaddr raddr;
  union {
    int socket;
#if HAVE_RDMA
    struct rdma_cm_id *cm_id;
#endif
  };
  geds::FileTransferProtocol type;
};

} // namespace geds

#endif
