/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TcpServer.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/strand.hpp>
#include <cstdint>

#include "Logging.h"
#include "TcpConnection.h"
#include "TcpDataTransport.h"

namespace geds {

constexpr size_t BUFFER_ALIGNMENT = 32;

using boost::asio::ip::tcp;

TcpServer::TcpServer(std::shared_ptr<GEDS> geds, uint16_t portArg)
    : _geds(geds), _buffers(MAXIMUM_TCP_THREADS()), port(portArg), _endpoint(tcp::v4(), port),
      _acceptor(_ioService, _endpoint) {
  for (size_t i = 0; i < MAXIMUM_TCP_THREADS(); i++) {
    _buffers.push(new (std::align_val_t(BUFFER_ALIGNMENT)) uint8_t[MIN_SENDFILE_SIZE]);
  }
}

TcpServer::~TcpServer() {
  uint8_t *buffer = nullptr;
  while (_buffers.pop(buffer)) {
    delete[] buffer;
  }
}

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

uint8_t *TcpServer::getBuffer() {
  uint8_t *result;
  auto success = _buffers.pop(result);
  if (!success) {
    return new (std::align_val_t(BUFFER_ALIGNMENT)) uint8_t[MIN_SENDFILE_SIZE];
  }
  return result;
}

void TcpServer::releaseBuffer(uint8_t *buffer) {
  auto success = _buffers.push(buffer);
  if (!success) {
    delete[] buffer;
  }
}

void TcpServer::accept() {
  _acceptor.async_accept( //
      boost::asio::make_strand(_ioService),
      [this](boost::system::error_code ec, tcp::socket socket) {
        if (ec) {
          if (this->_started) {
            LOG_ERROR("Unable to accept connection ", ec.message());
          }
          return;
        } else {
          LOG_DEBUG("Accepting connection");
          socket.set_option(boost::asio::ip::tcp::no_delay(true));
          socket.set_option(boost::asio::socket_base::send_buffer_size(65536));
          socket.set_option(boost::asio::socket_base::receive_buffer_size(65536));
          auto connection = TcpConnection::create(std::move(socket), this->_geds, *this);
          connection->start();
        }
        accept();
      });
}

} // namespace geds
