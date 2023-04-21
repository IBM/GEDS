/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_SERVICE_H
#define FILE_TRANSFER_SERVICE_H

#include <algorithm>
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <type_traits>
#include <vector>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <grpcpp/grpcpp.h>

#include "FileTransferProtocol.h"
#include "GEDSInternal.h"
#include "Object.h"
#include "RWConcurrentObjectAdaptor.h"
#include "TcpTransport.h"
#include "geds.grpc.pb.h"

class GEDS;

namespace geds {

struct RemoteFileInfo {
  const ObjectID id;
  const size_t length;
};

class FileTransferService: public utility::RWConcurrentObjectAdaptor {
  ConnectionState _connectionState;
  std::shared_ptr<grpc::Channel> _channel;
  std::unique_ptr<geds::rpc::GEDSService::Stub> _stub;
  std::shared_ptr<GEDS> _geds;
  std::shared_ptr<TcpTransport> _tcp;
  std::weak_ptr<TcpPeer> _tcpPeer;

  absl::StatusOr<std::vector<std::tuple<sockaddr, geds::FileTransferProtocol>>>
  availTransportEndpoints();

public:
  const std::string nodeAddress;

  FileTransferService(std::string nodeAddress, std::shared_ptr<GEDS> geds,
                      std::shared_ptr<TcpTransport> tcpTrans);
  ~FileTransferService();

  absl::Status connect();
  absl::Status disconnect();

  absl::StatusOr<std::vector<uint8_t>> get(const ObjectID &id, size_t position, size_t length);
  absl::StatusOr<std::vector<uint8_t>> get(const std::string &bucket, const std::string &key,
                                           size_t position, size_t length);

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

#endif
