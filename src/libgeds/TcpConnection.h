/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "TcpDataTransport.h"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/strand.hpp>
#include <cstddef>
#include <memory>
#include <vector>

#include <absl/status/status.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/bind/bind.hpp>

#include "GEDSFile.h"

class GEDS;

namespace geds {

class TcpServer;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {

  boost::asio::ip::tcp::socket _socket;
  std::shared_ptr<GEDS> _geds;

  boost::asio::strand<boost::asio::any_io_executor> _strand;
  TcpServer &_server;

  TcpConnection(boost::asio::ip::tcp::socket &&socket, std::shared_ptr<GEDS> geds,
                TcpServer &server);

  void awaitRequest();
  void handleWrite(const std::string &bucket, const std::string &key, size_t offset, size_t length);
  void handleWriteSendfile(GEDSFile file, int fd, int64_t offset, size_t count);
  void handleError(const absl::Status &status);

  boost::asio::streambuf _buffer;

public:
  static std::shared_ptr<TcpConnection> create(boost::asio::ip::tcp::socket &&socket,
                                               std::shared_ptr<GEDS> geds, TcpServer &server);

  boost::asio::ip::tcp::socket &socket() { return _socket; }

  void start();
};
} // namespace geds
