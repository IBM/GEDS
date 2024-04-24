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
#include <thread>
#include <vector>

#include "RWConcurrentObjectAdaptor.h"
#include "TcpConnection.h"

class GEDS;

namespace geds {

class TcpServer : public utility::RWConcurrentObjectAdaptor {
  bool _started = false;
  std::shared_ptr<GEDS> _geds;

  std::vector<std::thread> _threads;

public:
  const uint16_t port;

protected:
  boost::asio::io_service _ioService;

  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;

public:
  TcpServer(std::shared_ptr<GEDS> geds, uint16_t portArg);

  absl::Status start();
  void stop();

private:
  void accept();
};
} // namespace geds
