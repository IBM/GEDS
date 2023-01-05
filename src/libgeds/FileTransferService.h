/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <grpcpp/grpcpp.h>
#include <memory>

#include "ConcurrentQueue.h"
#include "FileTransferProtocol.h"
#include "GEDSInternal.h"
#include "TcpClient.h"
#include "geds.grpc.pb.h"

class GEDS;

namespace geds {

class FileTransferService {
  ConnectionState _connectionState = ConnectionState::Disconnected;
  std::shared_ptr<grpc::Channel> _channel;
  std::unique_ptr<geds::rpc::GEDSService::Stub> _stub;

  std::shared_ptr<GEDS> _geds;
  utility::ConcurrentQueue<std::shared_ptr<TcpClient>> _connections;

  absl::StatusOr<std::vector<std::tuple<std::string, uint16_t, FileTransferProtocol>>>
  availTransportEndpoints();

public:
  const std::string nodeAddress;

  FileTransferService(std::string nodeAddress, std::shared_ptr<GEDS> geds);
  ~FileTransferService();

  absl::Status connect();
  absl::Status disconnect();

  absl::StatusOr<size_t> readBytes(const std::string &bucket, const std::string &key,
                                   uint8_t *buffer, size_t position, size_t length);

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::StatusOr<size_t> read(const std::string &bucket, const std::string &key, T *buffer,
                              size_t position, size_t length) {
    size_t lengthBytes = length * sizeof(T);
    size_t positionBytes = position * sizeof(T);

    auto readStatus = // NOLINTNEXTLINE
        readBytes(bucket, key, reinterpret_cast<uint8_t *>(buffer), positionBytes, lengthBytes);
    if (!readStatus.ok()) {
      return readStatus.status();
    }
    return readStatus.value() / sizeof(T);
  }

  template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
  absl::StatusOr<size_t> read(const std::string &bucket, const std::string &key,
                              std::vector<T> &buffer, size_t offset, size_t position,
                              size_t length) {
    size_t requiredLength = offset + length;
    if (buffer.size() < requiredLength) {
      buffer.resize(requiredLength);
    }
    return read(bucket, key, &buffer[offset], position, length);
  }
};

} // namespace geds
