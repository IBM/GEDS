/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "FileTransferService.h"

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>

#include <absl/status/status.h>

#include "FileTransferProtocol.h"
#include "GEDSInternal.h"
#include "Logging.h"
#include "TcpClient.h"
#include "TcpDataTransport.h"
#include "geds.pb.h"

namespace geds {
using std::string;

#define CHECK_CONNECTED                                                                            \
  if (_connectionState != ConnectionState::Connected) {                                            \
    LOG_ERROR("Unable to connect");                                                                \
    return absl::FailedPreconditionError("Not connected.");                                        \
  }

FileTransferService::FileTransferService(std::string nodeAddress, std::shared_ptr<GEDS> geds)
    : _geds(geds), nodeAddress(std::move(nodeAddress)) {}

FileTransferService::~FileTransferService() {
  if (_connectionState == ConnectionState::Connected) {
    disconnect().IgnoreError();
  }
}

absl::Status FileTransferService::connect() {
  if (_connectionState != ConnectionState::Disconnected) {
    return absl::FailedPreconditionError("Cannot reinitialize service.");
  }
  try {
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

  auto it = nodeAddress.find(":");
  if (it == std::string::npos) {
    return absl::UnknownError("Unable to parse node address.");
  }
  auto nodeLoc = nodeAddress.substr(0, it);
  auto endpoints = availTransportEndpoints();
  if (!endpoints.ok()) {
    return endpoints.status();
  }

  for (size_t i = 0; i < geds::MAXIMUM_TCP_THREADS(); i++) {
    for (auto &ep : *endpoints) {
      if (std::get<2>(ep) == FileTransferProtocol::Socket) {
        const auto &ep_ip = std::get<0>(ep);
        if (ep_ip != nodeLoc) {
          continue;
        }
        auto ep_port = std::get<1>(ep);
        LOG_DEBUG("Creating a new TcpClient for ", ep_ip, ":", ep_port);
        auto connection = std::make_shared<TcpClient>(ep_ip, ep_port);
        auto status = connection->connect();
        if (!status.ok()) {
          connection = nullptr;
          continue;
        }
        _connections.push(connection);
        break;
      }
    }
  }

  _connectionState = ConnectionState::Connected;
  return absl::OkStatus();
}

absl::StatusOr<std::vector<std::tuple<std::string, uint16_t, FileTransferProtocol>>>
FileTransferService::availTransportEndpoints() {
  // Function is called during connect, so no check.
  geds::rpc::EmptyParams request;
  geds::rpc::AvailTransportEndpoints response;
  grpc::ClientContext context;

  auto status = _stub->GetAvailEndpoints(&context, request, &response);
  if (!status.ok()) {
    LOG_ERROR("Unable to execute grpc call, status: ", status.error_code(), " ",
              status.error_details());
    return absl::UnknownError("Unable to obtain available endpoints!");
  }

  const auto rpc_results = response.endpoint();
  auto results = std::vector<std::tuple<std::string, uint16_t, FileTransferProtocol>>{};
  for (const auto &i : rpc_results) {
    results.emplace_back(std::make_tuple(i.address(), i.port(),
                                         i.type() == rpc::RDMA ? FileTransferProtocol::RDMA
                                                               : FileTransferProtocol::Socket));
  }
  return results;
}

absl::Status FileTransferService::disconnect() {
  if (_connectionState != ConnectionState::Connected) {
    return absl::UnknownError("The service is in the wrong state!");
  }
  _connectionState = ConnectionState::Unknown;
  for (size_t i = 0; i < MAXIMUM_TCP_THREADS(); i++) {
    auto tcp = _connections.pop_wait_until_available();
    tcp = nullptr;
  }
  _channel = nullptr;
  _connectionState = ConnectionState::Disconnected;
  return absl::OkStatus();
}

absl::StatusOr<size_t> FileTransferService::readBytes(const std::string &bucket,
                                                      const std::string &key, uint8_t *buffer,
                                                      size_t position, size_t length) {
  CHECK_CONNECTED
  auto tcp = _connections.pop_wait_until_available();
  auto status = tcp->readBytes(bucket, key, buffer, position, length);
  _connections.push(tcp);
  return status;
}

} // namespace geds
