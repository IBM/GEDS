/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_GRPCSERVER_H
#define GEDS_GRPCSERVER_H

#include <memory>
#include <string>

#include <absl/status/status.h>
#include <grpcpp/grpcpp.h>

#include "MDSKVS.h"
#include "Ports.h"

class GRPCServer {
  std::shared_ptr<MDSKVS> _kvs;
  std::unique_ptr<grpc::Service> _grpcService;
  std::string _serverAddress;

public:
  GRPCServer(std::string serverAddress);
  GRPCServer() : GRPCServer("0.0.0.0:" + std::to_string(defaultMetdataServerPort)) {}
  ~GRPCServer();
  absl::Status startAndWait();
};

#endif // GEDS_GRPCSERVER_H
