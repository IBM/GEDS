/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "TcpDataTransport.h"

namespace geds {

class FileTransferService;
class TcpClient : public std::enable_shared_from_this<TcpClient> {

  std::string _ip;
  uint16_t _port;

  boost::asio::io_context _ioContext;
  std::unique_ptr<boost::asio::ip::tcp::socket> _socket;

public:
  TcpClient(std::string ip, uint16_t port);
  ~TcpClient();

  absl::Status connect();

  absl::StatusOr<size_t> readBytes(const std::string &bucket, const std::string &key,
                                   uint8_t *buffer, size_t position, size_t length);
};
} // namespace geds
