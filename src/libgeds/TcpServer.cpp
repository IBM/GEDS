/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TcpServer.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/strand.hpp>

#include "Logging.h"
#include "TcpDataTransport.h"
#include "TcpConnection.h"

namespace geds {

using boost::asio::ip::tcp;

TcpServer::TcpServer(std::shared_ptr<GEDS> geds, uint16_t portArg)
    : _geds(geds), port(portArg), _endpoint(tcp::v4(), port), _acceptor(_ioService, _endpoint) {}

absl::Status TcpServer::start() {
  if (_started) {
    return absl::UnknownError("Already started!");
  }

  accept();
  for (size_t i = 0; i < MAXIMUM_TCP_THREADS(); i++) {
    _threads.emplace_back(std::thread([&] { _ioService.run(); }));
  }
  _started = true;
  return absl::OkStatus();
}

void TcpServer::stop() {
  LOG_DEBUG("Stopping");
  _started = false;
  _acceptor.close();
  _ioService.stop();
  for (auto &t : _threads) {
    t.join();
  }
}

void TcpServer::accept() {
  _acceptor.async_accept( //
      boost::asio::make_strand(_ioService),
      [&, this](boost::system::error_code ec, tcp::socket socket) {
        if (ec) {
          if (_started) {
            LOG_ERROR("Unable to accept connection ", ec.message());
          }
          return;
        } else {
          LOG_DEBUG("Accepting connection");
          auto connection = TcpConnection::create(std::move(socket), _geds);
          connection->start();
        }
        accept();
      });
}

} // namespace geds
