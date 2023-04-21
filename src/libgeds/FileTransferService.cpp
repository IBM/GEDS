/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "FileTransferService.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "FileTransferProtocol.h"
#include "GEDS.h"
#include "GEDSInternal.h"
#include "Logging.h"
#include "Object.h"
#include "Status.h"
#include "TcpTransport.h"
#include "geds.grpc.pb.h"
#include "geds.pb.h"
#include "status.pb.h"

namespace geds {
using std::string;

#define CHECK_CONNECTED                                                                            \
  if (_connectionState != ConnectionState::Connected) {                                            \
    LOG_ERROR("Unable to connect");                                                                \
    return absl::FailedPreconditionError("Not connected.");                                        \
  }

FileTransferService::FileTransferService(std::string nodeAddress, std::shared_ptr<GEDS> geds,
                                         std::shared_ptr<TcpTransport> tcpTrans)
    : _connectionState(ConnectionState::Disconnected), _channel(nullptr), _geds(geds),
      _tcp(tcpTrans), nodeAddress(std::move(nodeAddress)) {}

FileTransferService::~FileTransferService() {
  if (_connectionState == ConnectionState::Connected) {
    disconnect().IgnoreError();
  }
}

absl::Status FileTransferService::connect() {
  if (_connectionState != ConnectionState::Disconnected) {
    return absl::FailedPreconditionError("Cannot reinitialize service.");
  }
  auto lock = getWriteLock();
  try {
    assert(_channel.get() == nullptr);
    _channel = grpc::CreateChannel(nodeAddress, grpc::InsecureChannelCredentials());
    auto success = _channel->WaitForConnected(grpcDefaultDeadline());
    if (!success) {
      return absl::UnavailableError("Could not connect to " + nodeAddress + ".");
    }
    _stub = geds::rpc::GEDSService::NewStub(_channel);
  } catch (const std::exception &e) {
    return absl::UnavailableError("Could not open channel with " + nodeAddress +
                                  ". Reason: " + e.what());
  }
  LOG_DEBUG("About to check for available file transfer endpoints");

  auto endpoints = availTransportEndpoints();
  for (auto &addr : *endpoints) {
    if (std::get<1>(addr) == FileTransferProtocol::Socket) {
      struct sockaddr saddr = std::get<0>(addr);
      auto peer = _tcp->getPeer(&saddr);

      if (peer) {
        _tcpPeer = peer;
        // for now, make just one connection
        break;
      }
    }
  }
  _connectionState = ConnectionState::Connected;
  return absl::OkStatus();
}

absl::Status FileTransferService::disconnect() {
  if (_connectionState != ConnectionState::Connected) {
    return absl::UnknownError("The service is in the wrong state!");
  }
  auto lock = getWriteLock();
  _tcpPeer.reset();
  _channel = nullptr;
  return absl::OkStatus();
}

absl::StatusOr<std::vector<std::tuple<sockaddr, FileTransferProtocol>>>
FileTransferService::availTransportEndpoints() {
  // Function is called during connect, so no check.
  geds::rpc::EmptyParams request;
  geds::rpc::AvailTransportEndpoints response;
  grpc::ClientContext context;

  auto status = _stub->GetAvailEndpoints(&context, request, &response);
  if (!status.ok()) {
    LOG_ERROR("Unable to execute grpc call, status: ", status.error_code(), " ",
              status.error_details());
    return absl::UnknownError("Unable to execute command");
  }

  const auto rpc_results = response.endpoint();
  auto results = std::vector<std::tuple<struct sockaddr, FileTransferProtocol>>();
  results.reserve(rpc_results.size());

  for (auto &i : rpc_results) {
    sockaddr saddr{};
    auto inaddr = (sockaddr_in *)&saddr;
    inaddr->sin_addr.s_addr = inet_addr(i.address().c_str());
    inaddr->sin_port = i.port();
    inaddr->sin_family = AF_INET;

    // For now only TCP connections are possible
    if (i.type() == rpc::Socket) {
      results.emplace_back(saddr, FileTransferProtocol::Socket);
    }
  }
  return results;
}

absl::StatusOr<size_t> FileTransferService::readBytes(const std::string &bucket,
                                                      const std::string &key, uint8_t *buffer,
                                                      size_t position, size_t length) {
  CHECK_CONNECTED

  std::future<absl::StatusOr<size_t>> fut;
  // Create a scope for the std::shared_ptr<TcpPeer> so that the peer is automatically cleaned up.
  {
    auto lock = getReadLock();
    if (_tcpPeer.expired()) {
      return absl::UnavailableError("No TCP for " + nodeAddress);
    }

    LOG_DEBUG("Found TCP peer for ", nodeAddress);
    auto peer = _tcpPeer.lock();
    lock.unlock();
    auto prom = peer->sendRpcRequest((uint64_t)buffer, bucket + "/" + key, position, length);
    fut = prom->get_future();
  }
  auto status = fut.get();
  if (status.ok()) {
    return *status;
  }
  // Close the FileTransferService on error.
  if (status.status().code() == absl::StatusCode::kAborted) {
    auto lock = getWriteLock();
    _tcpPeer.reset();
  }
  return status.status();
}

} // namespace geds
