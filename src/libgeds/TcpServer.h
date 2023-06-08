/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <set>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <boost/asio.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lockfree/stack.hpp>
#include <thread>
#include <vector>

#include "RWConcurrentObjectAdaptor.h"
#include "TcpConnection.h"

class GEDS;

namespace geds {

constexpr size_t MIN_SENDFILE_SIZE = 8192;

class TcpServer : public utility::RWConcurrentObjectAdaptor {
  bool _started = false;
  std::shared_ptr<GEDS> _geds;

  std::vector<std::thread> _threads;
  boost::lockfree::stack<uint8_t *, boost::lockfree::fixed_sized<false>> _buffers;

public:
  const uint16_t port;

protected:
  boost::asio::io_service _ioService;

  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;

public:
  TcpServer(std::shared_ptr<GEDS> geds, uint16_t portArg);
  ~TcpServer();

  uint8_t *getBuffer();
  void releaseBuffer(uint8_t *buffer);

  absl::Status start();
  void stop();

private:
  void accept();
};
} // namespace geds
