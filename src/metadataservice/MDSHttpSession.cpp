/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MDSHttpSession.h"

#include <sstream>

#include "Logging.h"
#include "Nodes.h"
#include "Statistics.h"

namespace geds {

MDSHttpSession::MDSHttpSession(boost::asio::ip::tcp::socket &&socket, Nodes &nodes)
    : _stream(std::move(socket)), _nodes(nodes) {}

MDSHttpSession::~MDSHttpSession() { close(); }

void MDSHttpSession::start() {
  LOG_DEBUG("Start connection");
  auto self = shared_from_this();
  boost::asio::dispatch(_stream.get_executor(),
                        boost::beast::bind_front_handler(&MDSHttpSession::awaitRequest, self));
}

void MDSHttpSession::awaitRequest() {
  auto self = shared_from_this();
  _request = {};
  _stream.expires_after(std::chrono::seconds(10));

  boost::beast::http::async_read(
      _stream, _buffer, _request,
      [self](boost::beast::error_code ec, std::size_t /* bytes_transferred */) {
        if (ec == boost::beast::http::error::end_of_stream) {
          return;
        }
        if (ec) {
          LOG_ERROR("Failed reading stream", ec.message());
          return;
        }
        self->handleRequest();
      });
}

void MDSHttpSession::prepareHtmlReply() {
  _response.result(boost::beast::http::status::ok);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "text/html");
  _response.keep_alive(_request.keep_alive());

  boost::beast::ostream(_response.body()) << "<!DOCTYPE html>"
                                          << "<html><head>"
                                          << "<title>GEDS Service</title>"
                                          << "</head>"
                                          << "<body>"
                                          << "<pre>"
                                          << "GEDS Metadata Service\n"
                                          << "=====================\n"
                                          << "\n";

  const auto &info = _nodes.information();
  if (info.size()) {
    boost::beast::ostream(_response.body()) << "Registered nodes:\n";
    info.forall([&](const std::shared_ptr<NodeInformation> &node) {
      const auto &[heartBeat, ts] = node->lastHeartBeat();
      boost::beast::ostream(_response.body())
          << " - " << node->identifier << " "                         //
          << "Allocated: " << heartBeat.storageAllocated << " "       //
          << "Used: " << heartBeat.storageUsed << " "                 //
          << "Memory Allocated: " << heartBeat.memoryAllocated << " " //
          << "Memory Used: " << heartBeat.memoryUsed << "\n";
    });
  }
  boost::beast::ostream(_response.body()) << "</pre>"
                                          << "</body></html>" << std::endl;
  handleWrite();
}

void MDSHttpSession::handleRequest() {
  if (_request.method() != boost::beast::http::verb::get) {
    return prepareError(boost::beast::http::status::bad_request, "Invalid method.");
  }
  if (_request.target().empty() || _request.target()[0] != '/') {
    return prepareError(boost::beast::http::status::bad_request, "Invalid path.");
  }

  if (_request.target() == "/") {
    return prepareHtmlReply();
  }
  if (_request.target() == "/metrics") {
    return prepareMetricsReply();
  }

  return prepareError(boost::beast::http::status::not_found, "Invalid path");
}

void MDSHttpSession::prepareMetricsReply() {
  _response.result(boost::beast::http::status::ok);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "plain/text");
  _response.keep_alive(_request.keep_alive());

  std::stringstream stream;
  Statistics::get().prometheusMetrics(stream);
  boost::beast::ostream(_response.body()) << stream.str();
  handleWrite();
}

void MDSHttpSession::prepareError(boost::beast::http::status status, std::string message) {
  _response.result(status);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "text/html");
  _response.keep_alive(_request.keep_alive());
  boost::beast::ostream(_response.body()) << message;

  return handleWrite();
}

void MDSHttpSession::handleWrite() {
  auto self = shared_from_this();
  _response.content_length(_response.body().size());

  boost::beast::http::async_write(
      _stream, _response,
      [self](boost::beast::error_code ec, std::size_t /* unused bytesTransferred */) {
        if (ec) {
          LOG_ERROR("Error ", ec.message());
          return;
        }
        if (self->_request.keep_alive()) {
          self->awaitRequest();
        }
        self->_buffer.clear();
      });
}

void MDSHttpSession::close() {
  LOG_DEBUG("Closing connection");

  boost::beast::error_code ec;
  _stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
  _stream.socket().close();
}

} // namespace geds
