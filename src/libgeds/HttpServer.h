/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "HttpSession.h"

#include <memory>
#include <thread>

#include <absl/status/status.h>
#include <boost/asio/io_context.hpp>

namespace geds {

class HttpServer {
  uint16_t _port;

  boost::asio::io_context _ioContext{1};
  std::unique_ptr<boost::asio::ip::tcp::acceptor> _acceptor = nullptr;
  std::thread _thread;

public:
  HttpServer(uint16_t port);
  absl::Status start();
  void stop();

private:
  void accept();
};

} // namespace geds
