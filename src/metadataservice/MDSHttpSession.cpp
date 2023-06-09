/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MDSHttpSession.h"

#include <exception>
#include <sstream>
#include <string>
#include <vector>

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/system_error.hpp>
#include <boost/json/value_from.hpp>
#include <magic_enum.hpp>

#include "Logging.h"
#include "MDSKVS.h"
#include "MDSKVSBucket.h"
#include "Nodes.h"
#include "Statistics.h"

void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                const std::shared_ptr<MDSKVSBucket> &n) {
  auto &nv = jv.emplace_array();
  n->forall([&nv](const utility::Path &key, const geds::ObjectInfo &info) {
    nv.push_back({key.name,
                  {{"location", info.location},
                   {"size", info.size},
                   {"metadata", info.metadata.has_value()
                                    ? (std::to_string(info.metadata->size()) + " bytes")
                                    : std::string{"<none>"}}}});
  });
}

void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                const std::shared_ptr<MDSKVS> &n) {
  auto &nv = jv.emplace_array();
  auto buckets = n->listBuckets();
  if (!buckets.ok()) {
    jv = nv;
    return;
  }
  for (const auto &bucket : *buckets) {
    auto objs = n->getBucket(bucket);
    if (!objs.ok()) {
      continue;
    }
    auto b = *objs;
    auto value = boost::json::value_from(b);
    nv.push_back({bucket, value});
  }
  jv = nv;
}

namespace geds {

MDSHttpSession::MDSHttpSession(boost::asio::ip::tcp::socket &&socket, Nodes &nodes,
                               std::shared_ptr<MDSKVS> kvs)
    : _stream(std::move(socket)), _nodes(nodes), _kvs(kvs) {}

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
          << " - " << node->uuid << ": "                                 //
          << node->host << ":" << node->port << " "                      //
          << std::string{magic_enum::enum_name(node->state())} << " -- " //
          << "Allocated: " << heartBeat.storageAllocated << " "          //
          << "Used: " << heartBeat.storageUsed << " "                    //
          << "Memory Allocated: " << heartBeat.memoryAllocated << " "    //
          << "Memory Used: " << heartBeat.memoryUsed << "\n";
    });
  }
  boost::beast::ostream(_response.body()) << "</pre>"
                                          << "</body></html>" << std::endl;
  boost::beast::ostream(_response.body()) << "\n";
  handleWrite();
}

void MDSHttpSession::prepareApiListReply() {
  _response.result(boost::beast::http::status::ok);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "application/json");
  _response.keep_alive(_request.keep_alive());

  auto data = boost::json::value_from(_kvs);
  boost::beast::ostream(_response.body()) << boost::json::serialize(data);
  boost::beast::ostream(_response.body()) << "\n";
  handleWrite();
}

void MDSHttpSession::prepareApiNodesReply() {
  _response.result(boost::beast::http::status::ok);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "application/json");
  _response.keep_alive(_request.keep_alive());

  auto data = boost::json::value_from(_nodes);
  boost::beast::ostream(_response.body()) << boost::json::serialize(data);
  boost::beast::ostream(_response.body()) << "\n";
  handleWrite();
}

void MDSHttpSession::prepareApiDecommissionReply(const std::string &body) {
  _response.result(boost::beast::http::status::ok);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "application/json");
  _response.keep_alive(_request.keep_alive());

  boost::json::error_code ec;
  auto parsed = boost::json::parse(body, ec);
  if (ec) {
    return prepareError(boost::beast::http::status::bad_request, ec.message());
  }
  if (!parsed.is_array()) {
    return prepareError(boost::beast::http::status::bad_request, "Expected array!");
  }

  std::vector<std::string> hostsToDecommission;
  for (const auto &value : parsed.as_array()) {
    if (!value.is_string()) {
      return prepareError(boost::beast::http::status::bad_request, "Unexpected element in array!");
    }
    hostsToDecommission.push_back(boost::json::value_to<std::string>(value));
  }

  auto status = _nodes.decommissionNodes(hostsToDecommission, _kvs);
  if (!status.ok()) {
    return prepareError(boost::beast::http::status::internal_server_error,
                        std::string{status.message()});
  }

  boost::beast::ostream(_response.body()) << R"({"status": "success", "nodes": )" << body << "}";
  boost::beast::ostream(_response.body()) << "\n";
  handleWrite();
}

void MDSHttpSession::prepareApiReregisterReply(const std::string &body) {
  _response.result(boost::beast::http::status::ok);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "application/json");
  _response.keep_alive(_request.keep_alive());

  // Parse body
  boost::json::error_code ec;
  auto parsed = boost::json::parse(body, ec);
  if (ec) {
    return prepareError(boost::beast::http::status::bad_request, ec.message());
  }
  if (!parsed.is_array()) {
    return prepareError(boost::beast::http::status::bad_request, "Expected array!");
  }

  // Send reply
  size_t count = 0;
  boost::beast::ostream(_response.body()) << R"({"status": "success", "nodes": [)";
  for (const auto &value : parsed.as_array()) {
    if (!value.is_string()) {
      continue;
    }
    auto host = boost::json::value_to<std::string>(value);
    auto status = _nodes.reregisterNode(host);
    if (status.ok()) {
      if (count > 1) {
        boost::beast::ostream(_response.body()) << ", ";
      }
      boost::beast::ostream(_response.body()) << "\"" << host << "\"";
      count++;
    }
  }

  boost::beast::ostream(_response.body()) << "\n";
  handleWrite();
}

void MDSHttpSession::handleRequest() {
  if (_request.target().empty() || _request.target()[0] != '/') {
    return prepareError(boost::beast::http::status::bad_request, "Invalid path.");
  }

  if (_request.method() == boost::beast::http::verb::get) {
    if (_request.target() == "/") {
      return prepareHtmlReply();
    }
    if (_request.target() == "/api/list") {
      return prepareApiListReply();
    }
    if (_request.target() == "/api/nodes") {
      return prepareApiNodesReply();
    }
    if (_request.target() == "/metrics") {
      return prepareMetricsReply();
    }
    return prepareError(boost::beast::http::status::not_found, "Invalid path");
  }
  if (_request.method() == boost::beast::http::verb::post) {
    auto body = boost::beast::buffers_to_string(_request.body().data());
    if (_request.target() == "/api/decommission") {
      return prepareApiDecommissionReply(body);
    }
    if (_request.target() == "/api/reregister") {
      return prepareApiReregisterReply(body);
    }
    return prepareError(boost::beast::http::status::not_found, "Invalid path");
  }
  return prepareError(boost::beast::http::status::bad_request, "Invalid method.");
}

void MDSHttpSession::prepareMetricsReply() {
  _response.result(boost::beast::http::status::ok);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "plain/text");
  _response.keep_alive(_request.keep_alive());

  std::stringstream stream;
  Statistics::get().prometheusMetrics(stream);
  boost::beast::ostream(_response.body()) << stream.str();
  boost::beast::ostream(_response.body()) << "\n";
  handleWrite();
}

void MDSHttpSession::prepareError(boost::beast::http::status status, std::string message) {
  _response.result(status);
  _response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  _response.set(boost::beast::http::field::content_type, "text/html");
  _response.keep_alive(_request.keep_alive());
  boost::beast::ostream(_response.body()) << message << "\n";

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
