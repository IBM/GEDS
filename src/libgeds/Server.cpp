/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Server.h"

#include <arpa/inet.h>
#include <cstdint>
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
    LOG_DEBUG << "About to report locally available file transfer endpoints" << std::endl;

    for (auto &endpoint : _server.TcpListenEp) {
      auto ep = response->add_endpoint();
      auto laddr = (sockaddr_in *)&endpoint.laddr;
      if (endpoint.type == FileTransferProtocol::RDMA) {
        ep->set_type(rpc::RDMA);
      } else {
        ep->set_type(rpc::Socket);
      }
      ep->set_address(endpoint.hostname);
      ep->set_port(laddr->sin_port);
      LOG_DEBUG << "Report local endpoint: " << inet_ntoa(laddr->sin_addr)
                << "::" << laddr->sin_port << std::endl;
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

void Server::TcpListenThread() {
  struct ObjTransferEndpoint listener;
  auto *local = (sockaddr_in *)&listener.laddr;
  socklen_t addrlen = sizeof(sockaddr_in);
  int sock, rv;

  listener.type = FileTransferProtocol::Socket;
  /*
   * Start TCP listener(s).
   *
   * For now just do one wildcard listen over all available IP interfaces
   * Let the kernel choose a free port for listening.
   */
  sock = ::socket(AF_INET, SOCK_STREAM, 0);
  if (sock <= 0) {
    LOG_ERROR << "socket call" << std::endl;
    return;
  }
  listener.socket = sock;

  memset(&listener.laddr, 0, sizeof listener.laddr);
  local->sin_family = AF_INET;
  local->sin_addr.s_addr = INADDR_ANY;

  rv = ::bind(sock, &listener.laddr, sizeof listener.laddr);
  if (rv) {
    LOG_ERROR << "bind call: " << std::endl;
    perror("bind");
    close(sock);
    return;
  }
  rv = getsockname(sock, &listener.laddr, &addrlen);
  if (rv) {
    LOG_ERROR << "getsockname call" << std::endl;
    close(sock);
    return;
  }
  /*
   * Remember hostname as local addr for now, since we do wildcard listen
   */
  auto hostname = utility::platform::getHostName();
  struct addrinfo *ainfo, ahints{};
  ahints.ai_family = AF_UNSPEC;
  ahints.ai_socktype = SOCK_STREAM;
  ahints.ai_protocol = IPPROTO_TCP;
  if (getaddrinfo(hostname.c_str(), nullptr, &ahints, &ainfo) == 0) {
    local->sin_addr.s_addr = ((sockaddr_in *)ainfo[0].ai_addr)->sin_addr.s_addr;
    listener.hostname = inet_ntoa(local->sin_addr);
    freeaddrinfo(ainfo);
  } else {
    perror("getaddrinfo");
    close(sock);
    return;
  }
  rv = ::listen(sock, 20);
  if (rv) {
    LOG_ERROR << "listen call" << std::endl;
    close(sock);
    return;
  }
  LOG_DEBUG << "TCP listener: " << listener.hostname << "::" << local->sin_port << std::endl;
  TcpListenEp.emplace(TcpListenEp.end(), listener);

  while (!TcpListenEp.empty()) {
    int newsock = ::accept(sock, nullptr, nullptr);
    if (newsock < 0) {
      perror("accept: ");
      continue;
    }
    if (_geds->_tcpTransport->addEndpointPassive(newsock) == false) {
      ::close(newsock);
      LOG_ERROR << "Server: Adding new TCP client failed " << std::endl;
    }
  }
  close(sock);
}

uint16_t Server::port() { return _port; }

absl::Status Server::start(std::shared_ptr<GEDS> geds) {
  if (_state == geds::ServiceState::Running) {
    return absl::FailedPreconditionError("The server is already running!");
  }

  _geds = geds;

  _listenThread = std::make_unique<std::thread>([this] { this->TcpListenThread(); });

  _grpcService = std::unique_ptr<grpc::Service>(new ServerImpl(geds, *this));

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  auto address = _hostname;

  int selectedPort = 0;
  do {
    grpc::ServerBuilder builder;
    LOG_DEBUG << "Trying port " << _port << std::endl;
    builder.AddListeningPort(address + ":" + std::to_string(_port),
                             grpc::InsecureServerCredentials(), &selectedPort);
    builder.RegisterService(_grpcService.get());
    builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
    _port++;
    _grpcServer = builder.BuildAndStart();
  } while (selectedPort == 0);
  _port = selectedPort;
  LOG_INFO << "GRPC Server started using " << _hostname << " and port " << _port << std::endl;

  _state = ServiceState::Running;
  // TODO: Check if _grpcServer->Wait() is required.
  _state = ServiceState::Running;
  return absl::OkStatus();
}

absl::Status Server::stop() {
  CHECK_SERVICE_RUNNING
  _grpcServer->Shutdown();
  _state = ServiceState::Stopped;
  return absl::OkStatus();
}

} // namespace geds
