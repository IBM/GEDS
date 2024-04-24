/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TcpClient.h"

#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

#include <absl/status/status.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include "FileTransferService.h"
#include "Logging.h"
#include "TcpDataTransport.h"

using boost::asio::ip::tcp;

namespace geds {

TcpClient::TcpClient(std::string ip, uint16_t port) : _ip(std::move(ip)), _port(port) {}

TcpClient::~TcpClient() {
  if (!_ioContext.stopped()) {
    _ioContext.stop();
  }
}

absl::StatusOr<size_t> TcpClient::readBytes(const std::string &bucket, const std::string &key,
                                            uint8_t *buffer, size_t position, size_t length) {
  {
    LOG_DEBUG("Requesting ", bucket, "/", key);
    auto request = tcp_transport::createGetRequest(bucket, key, position, length);
    LOG_DEBUG("Request: ", request);
    auto sendSize = request.size() + 1;
    auto rc = boost::asio::write(*_socket, boost::asio::buffer(request.data(), sendSize));
    if (rc != sendSize) {
      return absl::UnknownError("TcpClient sent an unexpected length!");
    }
  }

  {
    LOG_DEBUG("Waiting for response ");
    geds::tcp_transport::Response response;
    auto rc = boost::asio::read(*_socket, boost::asio::buffer(&response, sizeof(response)));
    if (rc != sizeof(response)) {
      return absl::UnknownError("TcpClient received an invalid amount of data!");
    }

    // Error case.
    if (response.statusCode != absl::OkStatus().raw_code()) {
      boost::asio::streambuf buf;
      rc = boost::asio::read(*_socket, buf.prepare(response.length));
      if (rc != response.length) {
        return absl::UnknownError("TcpClient received an unexpected length!");
      }
      buf.commit(response.length);

      std::string str(boost::asio::buffers_begin(buf.data()),
                      boost::asio::buffers_begin(buf.data()) + response.length);
      return absl::Status(static_cast<absl::StatusCode>(response.statusCode), str);
    }

    if (response.length > 0) {
      LOG_DEBUG("Reading the response ", response.length);
      rc = boost::asio::read(*_socket, boost::asio::buffer(buffer, response.length));
      if (rc != response.length) {
        return absl::UnknownError("TcpClient received an unexpected length!");
      }
    }
    return response.length;
  }
}

absl::Status TcpClient::connect() {
  LOG_DEBUG("Using ", _ip, " port ", _port, " to resolve the endpoint.");
  try {
    tcp::resolver resolver(_ioContext);
    auto endpoints = resolver.resolve(_ip, std::to_string(_port));
    _socket = std::make_unique<boost::asio::ip::tcp::socket>(_ioContext);

    boost::system::error_code ec;
    boost::asio::connect(*_socket, endpoints, ec);
    if (ec || !_socket->is_open()) {
      return absl::UnavailableError("Unable to connect " + ec.to_string());
    }
  } catch (std::exception &e) {
    return absl::UnknownError(e.what());
  }
  return absl::OkStatus();
}

} // namespace geds
