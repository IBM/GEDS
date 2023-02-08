/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HttpServer.h"

#include <memory>
#include <thread>

#include <absl/status/status.h>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>

#include "HttpSession.h"
#include "Logging.h"

namespace geds {

HttpServer::HttpServer(uint16_t port) : _port(port) {}

absl::Status HttpServer::start() {
  if (_acceptor != nullptr) {
    return absl::UnknownError("The server is already running!");
  }
  auto host = boost::asio::ip::make_address("0.0.0.0");
  _acceptor = std::unique_ptr<boost::asio::ip::tcp::acceptor>(
      new boost::asio::ip::tcp::acceptor(_ioContext, {host, _port}));
  _thread = std::thread([&] {
    accept();
    _ioContext.run();
  });
  return absl::OkStatus();
}

void HttpServer::stop() {
  _ioContext.stop();
  _acceptor->close();
  _thread.join();
}

void HttpServer::accept() {
  _acceptor->async_accept(boost::asio::make_strand(_ioContext),
                          [&](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
                            if (ec) {
                              LOG_ERROR("Unable to accept ", ec.message(), " ABORT.");
                              return;
                            }
                            std::make_shared<HttpSession>(std::move(socket))->start();
                            accept();
                          });
}

} // namespace geds
