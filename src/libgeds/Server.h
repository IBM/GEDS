/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#ifndef GEDS_SERVER_H
#define GEDS_SERVER_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <thread>

#include <absl/status/status.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>

#include "FileTransferProtocol.h"
#include "GEDSInternal.h"

class GEDS;

namespace geds {

/**
 * @brief Implements the GRPC services exposed by the GEDS instance.
 *
 */
class Server {
  std::shared_ptr<GEDS> _geds;

  geds::ServiceState _state;
  std::string _hostname;
  uint16_t _port;

  std::unique_ptr<grpc::Service> _grpcService;
  std::unique_ptr<grpc::Server> _grpcServer;

  std::unique_ptr<std::thread> _listenThread;
  void TcpListenThread();

public:
  Server(std::string hostname, std::optional<uint16_t> port = std::nullopt);
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  ~Server();

  uint16_t port();

  /**
   * @brief List of TCP connections for object transfers between server and client
   */
  std::list<geds::ObjTransferEndpoint> TcpListenEp;

  absl::Status start(std::shared_ptr<GEDS> geds);
  absl::Status stop();
};

} // namespace geds

#endif
