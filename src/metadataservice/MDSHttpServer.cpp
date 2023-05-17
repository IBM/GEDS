/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MDSHttpServer.h"

#include <memory>
#include <thread>

#include <absl/status/status.h>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception_ptr.hpp>

#include "Logging.h"
#include "MDSHttpSession.h"
#include "Nodes.h"

namespace geds {

MDSHttpServer::MDSHttpServer(uint16_t port, Nodes &nodes, std::shared_ptr<MDSKVS> kvs)
    : _port(port), _nodes(nodes), _kvs(kvs) {}

absl::Status MDSHttpServer::start() {
  if (_acceptor != nullptr) {
    return absl::UnknownError("The server is already running!");
  }
  try {
    auto host = boost::asio::ip::make_address("0.0.0.0");
    _acceptor = std::unique_ptr<boost::asio::ip::tcp::acceptor>(
        new boost::asio::ip::tcp::acceptor(_ioContext, {host, _port}));
    _thread = std::thread([&] {
      accept();
      _ioContext.run();
    });
  } catch (boost::exception &e) {
    // Workaround until GEDS properly supports multiple processes.
    auto diag = boost::diagnostic_information(e, false);
    return absl::InternalError("Unable to start webserver: " + diag);
  }
  return absl::OkStatus();
}

void MDSHttpServer::stop() {
  _ioContext.stop();
  _acceptor->close();
  _thread.join();
}

void MDSHttpServer::accept() {
  _acceptor->async_accept(
      boost::asio::make_strand(_ioContext),
      [&](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (ec) {
          LOG_ERROR("Unable to accept ", ec.message(), " ABORT.");
          return;
        }
        std::make_shared<MDSHttpSession>(std::move(socket), _nodes, _kvs)->start();
        accept();
      });
}

} // namespace geds
