/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Server.h"

#include <arpa/inet.h>
#include <cstdint>
#include <exception>
#include <ifaddrs.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <utility>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/status_code_enum.h>

#include "GEDS.h"
#include "GEDSInternal.h"
#include "Logging.h"
#include "Object.h"
#include "Platform.h"
#include "Ports.h"
#include "Status.h"
#include "TcpServer.h"
#include "geds.grpc.pb.h"
#include "geds.pb.h"

#include "FileTransferProtocol.h"

namespace geds {

class ServerImpl final : public geds::rpc::GEDSService::Service {
  std::shared_ptr<GEDS> _geds;
  Server &_server;

  ~ServerImpl() = default;

  ::grpc::Status GetAvailEndpoints(::grpc::ServerContext * /* unused context */,
                                   const ::geds::rpc::EmptyParams * /* unused request */,
                                   ::geds::rpc::AvailTransportEndpoints *response) override {
    LOG_DEBUG("About to report locally available file transfer endpoints");

    for (const auto &endpoint : _server.getAvailableEndpoints()) {
      auto ep = response->add_endpoint();
      auto type = std::get<0>(endpoint);
      auto addr = std::get<1>(endpoint);
      auto port = std::get<2>(endpoint);
      if (type == FileTransferProtocol::RDMA) {
        ep->set_type(rpc::RDMA);
      } else {
        ep->set_type(rpc::Socket);
      }
      ep->set_address(addr);
      ep->set_port(port);
    }
    return grpc::Status::OK;
  }

public:
  ServerImpl(std::shared_ptr<GEDS> geds, Server &server) : _geds(geds), _server(server) {}

protected:
};

Server::Server(std::string hostname, std::optional<uint16_t> port)
    : _state(ServiceState::Stopped), _hostname(std::move(hostname)),
      _port(port.value_or(defaultGEDSPort)) {
  if (_port == 0) {
    _port = defaultGEDSPort;
  }
}

Server::~Server() { (void)stop(); }

#define CHECK_SERVICE_RUNNING                                                                      \
  if (_state != ServiceState::Running) {                                                           \
    return absl::FailedPreconditionError("The service is " + to_string(_state) + ".");             \
  }

absl::Status Server::start(std::shared_ptr<GEDS> geds) {
  if (_state == geds::ServiceState::Running) {
    return absl::FailedPreconditionError("The server is already running!");
  }

  _geds = geds;
  _grpcService = std::unique_ptr<grpc::Service>(new ServerImpl(geds, *this));

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  auto address = _hostname;

  int selectedPort = 0;
  do {
    grpc::ServerBuilder builder;
    LOG_DEBUG("Trying port ", _port);
    builder.AddListeningPort(address + ":" + std::to_string(_port),
                             grpc::InsecureServerCredentials(), &selectedPort);
    builder.RegisterService(_grpcService.get());
    builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
    _port++;
    _grpcServer = builder.BuildAndStart();
  } while (selectedPort == 0);
  _port = selectedPort;
  LOG_INFO("GRPC Server started using ", _hostname, " and port ", _port);

  _state = ServiceState::Running;

  absl::Status status;
  size_t retryCount = 0;
  int socketServerPort = _port;
  do {
    retryCount++;
    socketServerPort++;
    try {
      _TcpServer = std::make_unique<TcpServer>(geds, socketServerPort);
      status = _TcpServer->start();
    } catch (std::exception &e) {
      status = absl::UnknownError("Error starting the socket transport " + std::string{e.what()});
    }
  } while (retryCount < 10 && !status.ok());

  if (!status.ok()) {
    return status;
  }

  {
    ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
      return absl::UnknownError("Could not query iffaddrs.");
    }

    for (auto addr = ifaddr; addr != nullptr; addr = addr->ifa_next) {
      if (addr->ifa_addr == nullptr) {
        continue;
      }
      // Check if interface is up
      if (!(addr->ifa_flags & IFF_UP)) {
        continue;
      }
      // Filter IPv4
      if (addr->ifa_addr->sa_family == AF_INET) {
        char arr[INET_ADDRSTRLEN];
        auto sa = reinterpret_cast<sockaddr_in *>(addr->ifa_addr);
        inet_ntop(AF_INET, &sa->sin_addr, &arr[0], INET_ADDRSTRLEN);
        std::string str(&arr[0]);
        if (str == "127.0.0.1") {
          continue;
        }
        LOG_DEBUG("Exporting ", str, ":", _TcpServer->port);
        _endpoints.emplace_back(
            std::make_tuple(FileTransferProtocol::Socket, str, _TcpServer->port));
      }
    }
    freeifaddrs(ifaddr);
  }

  // TODO: Check if _grpcServer->Wait() is required.
  _state = ServiceState::Running;
  return absl::OkStatus();
}

absl::Status Server::stop() {
  CHECK_SERVICE_RUNNING
  _TcpServer->stop();
  _TcpServer = nullptr;
  _grpcServer->Shutdown();
  _state = ServiceState::Stopped;
  return absl::OkStatus();
}

} // namespace geds
