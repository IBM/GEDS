/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <memory>
#include <thread>

#include <absl/status/status.h>
#include <boost/asio/io_context.hpp>

#include "MDSHttpSession.h"

class Nodes;

namespace geds {

class MDSHttpServer {
  uint16_t _port;

  boost::asio::io_context _ioContext{1};
  std::unique_ptr<boost::asio::ip::tcp::acceptor> _acceptor = nullptr;
  std::thread _thread;

  Nodes &_nodes;

public:
  MDSHttpServer(uint16_t port, Nodes &nodes);
  absl::Status start();
  void stop();

private:
  void accept();
};

} // namespace geds
