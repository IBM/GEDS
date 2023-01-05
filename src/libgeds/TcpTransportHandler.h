/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "TcpDataTransport.h"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <vector>

#include <absl/status/status.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/bind/bind.hpp>

class GEDS;

namespace geds {
class TcpTransportHandler : public std::enable_shared_from_this<TcpTransportHandler> {

  boost::asio::ip::tcp::socket _socket;
  std::shared_ptr<GEDS> _geds;

  boost::asio::strand<boost::asio::any_io_executor> _strand;

  TcpTransportHandler(boost::asio::ip::tcp::socket &&socket, std::shared_ptr<GEDS> geds);

  void awaitRequest();
  void handleWrite(const std::string &bucket, const std::string &key, size_t offset, size_t length);
  void handleError(const absl::Status &status);

  boost::asio::streambuf _buffer;

public:
  static std::shared_ptr<TcpTransportHandler> create(boost::asio::ip::tcp::socket &&socket,
                                                     std::shared_ptr<GEDS> geds);

  boost::asio::ip::tcp::socket &socket() { return _socket; }

  void start();
};
} // namespace geds
