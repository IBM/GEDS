/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/version.hpp>

#include <memory>
#include <string>

class Nodes;

namespace geds {

class MDSHttpSession : public std::enable_shared_from_this<MDSHttpSession> {
  boost::beast::tcp_stream _stream;
  boost::beast::flat_buffer _buffer{4096};
  boost::beast::http::request<boost::beast::http::dynamic_body> _request;
  boost::beast::http::response<boost::beast::http::dynamic_body> _response;

  Nodes &_nodes;

public:
  MDSHttpSession(boost::asio::ip::tcp::socket &&socket, Nodes &nodes);
  ~MDSHttpSession();
  void start();

  void awaitRequest();
  void handleRequest();
  void prepareHtmlReply();
  void prepareMetricsReply();
  void prepareApiNodesReply();
  void prepareError(boost::beast::http::status status, std::string message);
  void handleWrite();

  void close();
};
} // namespace geds
