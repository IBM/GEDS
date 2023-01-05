/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TcpTransportHandler.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <iterator>
#include <memory>
#include <ostream>
#include <sstream>
#include <vector>

#include <absl/status/status.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include "GEDS.h"
#include "Logging.h"
#include "TcpDataTransport.h"

namespace geds {

TcpTransportHandler::TcpTransportHandler(boost::asio::ip::tcp::socket &&socket,
                                         std::shared_ptr<GEDS> geds)
    : _socket(std::move(socket)), _geds(geds),
      _strand(boost::asio::make_strand(socket.get_executor())) {
  LOG_DEBUG("Creating connection on ", _socket.remote_endpoint().address().to_string(), ":",
            _socket.remote_endpoint().port());
}

std::shared_ptr<TcpTransportHandler>
TcpTransportHandler::create(boost::asio::ip::tcp::socket &&socket, std::shared_ptr<GEDS> geds) {
  return std::shared_ptr<TcpTransportHandler>(new TcpTransportHandler(std::move(socket), geds));
}

void TcpTransportHandler::start() {
  LOG_DEBUG("Starting connection");
  awaitRequest();
}

void TcpTransportHandler::awaitRequest() {
  auto self = shared_from_this();

  boost::asio::async_read_until( //
      _socket, _buffer, '\0',
      boost::asio::bind_executor(
          _strand, [self](boost::system::error_code ec, std::size_t /* bytes_transferred */) {
            if (ec) {
              return;
            }
            std::string requestStr((std::istreambuf_iterator<char>(&self->_buffer)),
                                   std::istreambuf_iterator<char>());
            self->_buffer.consume(self->_buffer.size());
            LOG_DEBUG("Request: '", requestStr, "'");

            std::string bucket;
            std::string key;
            size_t offset = 0;
            size_t length = 0;
            auto status = tcp_transport::parseGetRequest(requestStr, bucket, key, offset, length);

            if (!status.ok()) {
              self->handleError(status);
            } else {
              self->handleWrite(bucket, key, offset, length);
            }
          }));
}

void TcpTransportHandler::handleWrite(const std::string &bucket, const std::string &key,
                                      size_t offset, size_t length) {
  geds::tcp_transport::Response response;
  response.statusCode = absl::OkStatus().raw_code();
  response.length = 0;

  uint8_t *byteBuffer = nullptr;
  auto file = _geds->open(bucket, key);
  if (!file.ok()) {
    LOG_DEBUG("Unable to open ", bucket, "/", key, ": ", file.status().message());
    handleError(file.status());
    return;
  }

  std::vector<boost::asio::const_buffer> writeArray;
  writeArray.emplace_back(boost::asio::buffer(&response, sizeof(response)));

  if (file->rawPtr().ok()) {
    auto size = file->size();
    response.length = offset > size ? 0 : (std::min(size - offset, length));
    if (offset > size) {
      response.length = 0;
    }
    if (response.length > 0) {
      auto rawPtr = file->rawPtr();
      writeArray.emplace_back(boost::asio::buffer(&(*rawPtr)[offset], response.length));
    }
  } else {
    byteBuffer = new uint8_t[length];
    auto size = file->read(byteBuffer, offset, length);
    if (!size.ok()) {
      handleError(size.status());
      return;
    }
    response.length = *size;
    if (*size) {
      writeArray.emplace_back(boost::asio::buffer(byteBuffer, *size));
    }
  }

  LOG_DEBUG("Sending payload ", response.length);
  // Note: buffer, file need to be captured by the lambda.
  auto self = shared_from_this();
  boost::asio::async_write( //
      _socket, writeArray,  //
      [self, &writeArray, byteBuffer, &response, file](boost::system::error_code ec,
                                                       std::size_t /* length */) {
        if (byteBuffer) {
          delete[] byteBuffer;
        }
        if (ec) {
          LOG_ERROR("Error during write of ", file->identifier(), ": ", ec);
          return;
        }
        (void)response;
        (void)writeArray;

        LOG_DEBUG("Finished writing");
        self->awaitRequest();
      });
}

void TcpTransportHandler::handleError(const absl::Status &status) {
  LOG_DEBUG(status.message());

  geds::tcp_transport::Response response;
  response.statusCode = status.raw_code();
  response.length = status.message().size();
  std::string errorMessage = std::string{status.message()};

  auto buffers = std::vector<boost::asio::const_buffer>();
  buffers.emplace_back(boost::asio::buffer(&response, sizeof(response)));
  buffers.emplace_back(boost::asio::buffer(errorMessage.data(), errorMessage.size()));

  LOG_DEBUG("Sending payload ", response.length);
  auto self = shared_from_this();
  boost::asio::async_write( //
      _socket, buffers,     //
      [self, &buffers, &errorMessage, &response](boost::system::error_code ec, std::size_t) {
        if (ec) {
          LOG_ERROR("Error during write: ", ec);
          return;
        }
        (void)response;
        (void)buffers;
        (void)errorMessage;
        LOG_DEBUG("Finished writing");
        self->awaitRequest();
      });
}

} // namespace geds
