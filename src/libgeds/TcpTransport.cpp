/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <aws/common/thread.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <string>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <thread>
#include <type_traits>
#include <unistd.h>

#include "FileTransferProtocol.h"
#include "GEDS.h"
#include "GEDSFile.h"
#include "GEDSInternal.h"
#include "Logging.h"
#include "Object.h"
#include "TcpTransport.h"

constexpr size_t MIN_SENDFILE_SIZE = 4096;
constexpr size_t BUFFER_ALIGNMENT = 32;

namespace geds {

TcpTransport::TcpTransport(std::shared_ptr<GEDS> geds) : _geds(geds) {}

TcpTransport::~TcpTransport() { isServing = false; }

void TcpTransport::start() {
  if (isServing) {
    LOG_ERROR("TCP service already started");
    return;
  }

  u_int64_t registers[4]; // NOLINT
  __asm__ __volatile__("cpuid "
                       : "=a"(registers[0]), "=b"(registers[1]), "=c"(registers[2]),
                         "=d"(registers[3])
                       : "a"(1), "c"(0));
  bool hyperthreading = registers[3] & (1 << 28); // NOLINT
  num_proc = std::thread::hardware_concurrency();
  if (hyperthreading)
    num_proc /= 2;

  num_proc = std::min(num_proc, MAX_IO_THREADS);
  isServing = true;

  for (unsigned int id = 0; id < MAX_IO_THREADS; id++) {
    txThreads.push_back(std::make_unique<std::thread>([this, id] { this->tcpTxThread(id); }));
    rxThreads.push_back(std::make_unique<std::thread>([this, id] { this->tcpRxThread(id); }));
    _buffers.push(new (std::align_val_t(BUFFER_ALIGNMENT)) uint8_t[MIN_SENDFILE_SIZE]);
  }
  ioStatsThread = std::make_unique<std::thread>([this] { this->updateIoStats(); });

  LOG_DEBUG("TCP service start");
}

void TcpTransport::stop() {
  isServing = false;

  LOG_DEBUG("Stopping TCP Service");

  std::vector<std::shared_ptr<TcpPeer>> tcpPeerV;
  tcpPeers.forall([&tcpPeerV](std::shared_ptr<TcpPeer> &tp) { tcpPeerV.push_back(tp); });
  for (auto &ep : tcpPeerV)
    ep->cleanup();

  for (auto &t : txThreads)
    t->join();

  for (auto &t : rxThreads)
    t->join();

  // Introduces a performance regression in the I/O Benchmark.
  // ioStatsThread->join();
  // ioStatsThread = nullptr;

  tcpPeers.clear();
  txThreads.clear();
  rxThreads.clear();

  uint8_t *buffer;
  while (_buffers.pop(buffer)) {
    delete[] buffer;
  }
  LOG_DEBUG("TCP Transport stopped");
}

TcpPeer::~TcpPeer() {
  cleanup();
  assert(!endpoints.size());
}

void TcpPeer::cleanup() {
  epMux.lock();
  for (auto &endpoint : endpoints) {
    auto tep = endpoint.second;
    shutdown(tep->sock, SHUT_RDWR);
    LOG_DEBUG("Endpoint shutdown: socket: ", tep->sock, " sent: ", tep->tx_bytes,
              " received: ", tep->rx_bytes);
  }
  epMux.unlock();
}

bool TcpPeer::SocketTxReady(int sock) {
  bool rv = false;
  epMux.lock_shared();
  auto it = endpoints.find(sock);
  if (it->second) {
    auto tep = it->second;
    tep->send_ctx.stateMux.lock();
    rv = processEndpointSend(tep);
    tep->send_ctx.stateMux.unlock();
  }
  epMux.unlock_shared();
  return rv;
}

/**
 * @brief Write to socket until all sent or failure encountered.
 *
 * @param sock - socket to write to
 * @return true
 * @return false
 */
bool TcpPeer::processEndpointSend(std::shared_ptr<TcpEndpoint> tep) {
  struct TcpSendState *ctx = &tep->send_ctx;
  int sock = tep->sock;

  ssize_t sent = 0;

  do {
    ssize_t data_to_send = 0;
    sent = 0;
    uint16_t hdr_to_send = 0;
    struct iovec iov[2]; // NOLINT
    int ix = 0;

    if (ctx->state == PROC_IDLE) {
      auto workOpt = sendQueue.pop();
      if (!workOpt.has_value()) {
        // Stop processing
        return true;
      }
      (*sendQueue_stats)--;

      ctx->state = PROC_HDR;

      auto work = *workOpt;
      /*
       * Start processing new send work
       */
      ctx->hdr.reqid = work->reqId;
      ctx->hdr.hdrlen = sizeof ctx->hdr;
      ctx->objName.clear();
      if (work->objName.length()) {
        ctx->objName = work->objName;
        ctx->hdr.hdrlen += ctx->objName.length();
      }
      ctx->hdr.datalen = work->len;
      ctx->hdr.offset = work->off;
      ctx->hdr.type = work->type;
      ctx->va = work->va;
      ctx->in_fd = work->in_fd;
      ctx->progress = 0;
    }
    if (ctx->state == PROC_HDR) {
      if (ctx->progress < sizeof ctx->hdr) {
        hdr_to_send = sizeof ctx->hdr - ctx->progress;
        auto hdrp = reinterpret_cast<uint8_t *>(&ctx->hdr); // NOLINT
        iov[0].iov_base = &hdrp[ctx->progress];             // NOLINT
        iov[0].iov_len = hdr_to_send;
        ix = 1;
      }
      if (ctx->hdr.hdrlen > sizeof ctx->hdr) {
        int name_off = (ix == 1) ? 0 : ctx->progress - sizeof ctx->hdr;

        assert(ix == 1 || ctx->progress >= sizeof ctx->hdr);

        iov[ix].iov_base = ctx->objName.data() + name_off; // NOLINT
        iov[ix].iov_len = ctx->hdr.hdrlen - (sizeof ctx->hdr + name_off);
        hdr_to_send += iov[ix].iov_len;
        ix++;
      }
      sent = ::writev(sock, &iov[0], ix);
      if (sent >= 0) {
        tep->tx_bytes += sent;
        if (sent < hdr_to_send) {
          ctx->progress += sent;
          continue;
        } else {
          assert(sent == hdr_to_send);
          ctx->progress = 0;
        }
      } else {
        if (errno != EWOULDBLOCK)
          LOG_ERROR("Send failed, errno: ", errno);
        break;
      }
    }
    /*
     * Add more RPC types with payload
     *
     * If error is signalled back, no data are included.
     */
    if (ctx->hdr.type != GET_REPLY || ctx->hdr.error) {
      ctx->state = PROC_IDLE;
      continue;
    }
    data_to_send = ctx->hdr.datalen - ctx->progress;
    if (!data_to_send) {
      ctx->state = PROC_IDLE;
      continue;
    }
    ctx->state = PROC_DATA;

    if (ctx->in_fd > 0) {
      /*
       * use sendfile() for sending data
       */
      auto off = (off_t)(ctx->va + ctx->progress);
      off_t *offp = &off;
      /*
       * sendfile() does not maintain the read offset of
       * ctx->in_fd, if offp != NULL, so we do not change
       * the file's offset here.
       */
      sent = ::sendfile(sock, ctx->in_fd, offp, data_to_send);
    } else {
      auto vap = reinterpret_cast<uint8_t *>(ctx->va); // NOLINT
      iov[0].iov_base = &vap[ctx->progress];           // NOLINT
      iov[0].iov_len = data_to_send;
      sent = ::writev(sock, &iov[0], 1);
      /*
       * XXX This is a hack: Free buffer provided for RPC reply.
       * The caller should take care actually.
       */
      if (sent == data_to_send)
        _tcpTransport.releaseBuffer((uint8_t *)ctx->va);
    }
    if (sent == data_to_send) {
      tep->tx_bytes += ctx->hdr.datalen;
      ctx->state = PROC_IDLE;
      continue;
    } else if (sent < 0)
      break;
    ctx->progress += sent;
  } while (sent > 0);

  if (sent >= 0 || errno == EWOULDBLOCK)
    return true;

  ctx->state = PROC_FAILED;
  return false;
}

void TcpTransport::tcpTxThread(unsigned int id) {
  struct epoll_event events[EPOLL_MAXEVENTS]; // NOLINT

  LOG_DEBUG("TX thread ", id, " starting");

  int poll_fd = ::epoll_create1(0);
  if (poll_fd < 0) {
    perror("epoll_create: ");
    return;
  }
  epoll_wfd[id] = poll_fd;
  do {
    int cnt = ::epoll_wait(poll_fd, events, EPOLL_MAXEVENTS, -1);

    for (int i = 0; i < cnt; i++) {
      struct epoll_event *ev = &events[i];
      epoll_epid_t ep_id = {};
      ep_id.data = ev->data.u64;
      int sock = ep_id.id.sock;
      unsigned int epId = ep_id.id.peer_id;
      std::shared_ptr<TcpPeer> tcpPeer = nullptr;

      if (sock < 0) {
        LOG_ERROR("Invalid write socket: ", sock, " PeerId: ", ev->data.u64, ", evcnt: ", cnt);
        continue;
      }
      auto it = tcpPeers.get(epId);
      if (it.has_value()) {
        LOG_DEBUG("Found TX peer for: ", sock);
        tcpPeer = *it;
      } else {
        LOG_DEBUG("No TX peer for: ", sock);
        deactivateEndpoint(poll_fd, sock, ALL_CLOSED);
        continue;
      }
      if (ev->events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        deactivateEndpoint(poll_fd, sock, TX_CLOSED);
        if (tcpPeer->SocketStateChange(sock, TX_CLOSED)) {
          tcpPeers.remove(tcpPeer->Id);
        }
        continue;
      }
      if (!(ev->events & EPOLLOUT)) {
        LOG_DEBUG("No OUT: ", sock);
        continue;
      }
      if (tcpPeer->SocketTxReady(sock))
        continue;

      shutdown(sock, SHUT_RDWR);
      deactivateEndpoint(poll_fd, sock, TX_CLOSED);
      if (tcpPeer->SocketStateChange(sock, TX_CLOSED)) {
        tcpPeers.remove(tcpPeer->Id);
      }
    }
  } while (isServing);
  LOG_DEBUG("TX thread ", id, " exiting");
}

bool TcpPeer::SocketStateChange(int sock, uint32_t change) {
  epMux.lock();
  auto it = endpoints.find(sock);
  if (!it->second) {
    epMux.unlock();
    return true;
  }
  auto tep = it->second;
  bool dead = false;

  /*
   * Close socket for read or write
   */
  if (change && !(tep->state & change)) {
    tep->state |= change;
    shutdown(sock, SHUT_RDWR);
    if ((tep->state & ALL_CLOSED) == ALL_CLOSED) {
      dead = true;
      close(sock);
      endpoints.erase(it);
    }
  }
  if (endpoints.size() != 0)
    dead = false;
  epMux.unlock();

  return dead;
}

void TcpPeer::TcpProcessRpcGet(uint64_t reqId, const std::string objName, size_t len, size_t off) {
  auto separator = objName.find('/');
  if (separator == std::string::npos) {
    LOG_ERROR("cannot open file: ", objName, " invalid format!");
    sendRpcReply(reqId, -1, 0, 0, EINVAL);
    return;
  }

  auto bucket = objName.substr(0, separator);
  auto key = objName.substr(separator + 1);
  auto file = _geds->reOpen(bucket, key);
  if (!file.ok()) {
    LOG_ERROR("cannot open file: ", objName);
    sendRpcReply(reqId, -1, 0, 0, EINVAL);
    return;
  }
  auto filesize = file->size();
  if (off > filesize) {
    LOG_ERROR("offset > filesize: ", off, " > ", filesize);
    sendRpcReply(reqId, -1, 0, 0, EINVAL);
    return;
  }
  filesize -= off;
  len = (len == 0) ? filesize : std::min(filesize, len);
  if (len == 0) {
    sendRpcReply(reqId, -1, 0, 0, 0);
    return;
  }
  /*
   * Use sendfile() for large chunks of data
   */
  auto rawFd = file->rawFd();
  if (len >= MIN_SENDFILE_SIZE && rawFd.ok()) {
    int in_fd = *rawFd;
    sendRpcReply(reqId, in_fd, off, len, 0);
    return;
  }
  auto buffer = _tcpTransport.getBuffer();
  auto status = file->read(buffer, off, len);
  if (len != *status) {
    LOG_ERROR("file->read returned with an unexpected length!");
  }
  if (status.ok()) {
    sendRpcReply(reqId, -1, (uint64_t)buffer, *status, 0);
  } else {
    _tcpTransport.releaseBuffer(buffer);
    LOG_ERROR("cannot read file: ", objName);
    sendRpcReply(reqId, -1, 0, 0, EINVAL);
  }
}

/**
 * @brief Read socket until empty or read failure
 *
 * @param tep
 * @return true
 * @return false
 */
bool TcpPeer::processEndpointRecv(int sock) {
  std::shared_ptr<TcpEndpoint> tep;
  epMux.lock_shared();
  auto it = endpoints.find(sock);
  if (it->second) {
    tep = it->second;
    epMux.unlock_shared();
  } else {
    epMux.unlock_shared();
    LOG_ERROR("No peer for this endpoint: ", sock);
    return false;
  }
  TcpRcvState *ctx = &tep->recv_ctx;
  int op = -1;
  ssize_t rv = 0;

  do {
    bool start_data = false;

    if (ctx->state == PROC_IDLE) {
      memset(&ctx->hdr, 0, sizeof ctx->hdr);
      ctx->progress = 0;
      ctx->objName.clear();
      ctx->state = PROC_HDR;
    }
    if (ctx->state == PROC_HDR) {
      /*
       * Start or resume hdr reception
       */
      size_t to_recv = sizeof ctx->hdr - ctx->progress;

      while (ctx->progress < sizeof ctx->hdr) {
        auto hdrp = reinterpret_cast<uint8_t *>(&ctx->hdr);  // NOLINT
        rv = ::recv(sock, &hdrp[ctx->progress], to_recv, 0); // NOLINT
        if (rv <= 0) {
          rv = errno == EWOULDBLOCK ? -EAGAIN : (errno ? -errno : -EIO);
          break;
        }
        to_recv -= rv;
        ctx->progress += rv;
      }
      if (rv < 0)
        break;
      /*
       * Get the additional object name, if present
       */
      to_recv = ctx->hdr.hdrlen - ctx->progress;

      if (ctx->hdr.type == GET_REQ) {
        if (to_recv == 0 || to_recv > (ssize_t)RPC_TCP_MAX_HDR) {
          LOG_ERROR("RPC GET_REQ header size invalid: ", ctx->hdr.hdrlen);
          rv = -EINVAL;
          break;
        }
        char buffer[to_recv];

        while (to_recv) {
          rv = ::recv(sock, &buffer[0], to_recv, 0);
          if (rv <= 0) {
            rv = errno == EWOULDBLOCK ? -EAGAIN : (errno ? -errno : -EIO);
            break;
          }
          ctx->progress += rv;
          std::string name(buffer, rv);
          ctx->objName += name;
          to_recv -= rv;
        }
      } else if (to_recv) {
        int type = ctx->hdr.type, error = ctx->hdr.error;

        LOG_ERROR("RPC unexpected header content:: ", "reqid: ", ctx->hdr.reqid,
                  ", datalen: ", ctx->hdr.datalen, ", offset: ", ctx->hdr.offset,
                  ", hdrlen: ", ctx->hdr.hdrlen, ", type: ", type, ", error: ", error,
                  ", receive progress: ", ctx->progress);
        rv = -EINVAL;
        break;
      }
      if (rv < 0)
        break;
      ctx->state = PROC_DATA;
      tep->rx_bytes += ctx->progress;
      ctx->progress = 0;
      rv = 0;
      start_data = true;
    }

    op = ctx->hdr.type;
    unsigned long datalen = ctx->hdr.datalen;

    switch (op) {

    case GET_REQ:
      // Any error would be reported by RPC reply.
      TcpProcessRpcGet(ctx->hdr.reqid, ctx->objName, datalen, ctx->hdr.offset);

      ctx->state = PROC_IDLE;
      break;

    case GET_REPLY:
      if (start_data) {
        /*
         * Fetch corresponding work from receive queue
         */
        auto it = recvQueue.getAndRemove(ctx->hdr.reqid);
        if (!it.has_value()) {
          LOG_ERROR("Socket: ", sock, ", Peer: ", Id,
                    ": No corresponding receive for: ", ctx->hdr.reqid,
                    ", recv's pending: ", recvQueue.size());
          rv = -EINVAL;
          break;
        }
        (*recvQueue_stats)--;
        auto work = *it;
        ctx->va = work->va;
        ctx->p = work->p;

        if (ctx->hdr.error) {
          /*
           * The peer shall not send any data here.
           */
          if (datalen) {
            LOG_ERROR("Protocol failure, no data in error reply expected, but indicated: ", datalen,
                      " Ep: ", tep->sock);
            ctx->hdr.datalen = 0;
          }
          auto message = "Error during GET_REPLY: " + std::to_string(ctx->hdr.error) +
                         "length: " + std::to_string(datalen) + " Ep: " + std::to_string(tep->sock);
          ctx->p->set_value(absl::AbortedError(message));
          break;
        }
      }
      while (ctx->progress < datalen) {
        auto to_recv = datalen - ctx->progress;
        auto vap = reinterpret_cast<uint8_t *>(ctx->va);    // NOLINT
        rv = ::recv(sock, &vap[ctx->progress], to_recv, 0); // NOLINT
        if (rv < 0) {
          if (errno == EWOULDBLOCK) {
            return true;
          }
          ctx->state = PROC_FAILED;
          int err = errno;
          int eio = EIO;
          std::string message = "Error during recv: ";
          if (err) {
            message += "got errno " + std::to_string(err) + " " + strerror(err);
          } else {
            message += "got EIO " + std::to_string(eio);
          }
          ctx->p->set_value(absl::AbortedError(message));
          return false;
        }
        ctx->progress += rv;
      }
      if (ctx->progress == datalen) {
        /*
         * completed RPC exchange, inform caller
         */
        tep->rx_bytes += ctx->progress;
        ctx->p->set_value(datalen);
        ctx->state = PROC_IDLE;
      }
      break;
    default:
      LOG_ERROR("Unsupported RPC operation: ", op);
      return false;
    }
  } while (rv > 0);

  if (rv >= 0 || rv == -EAGAIN)
    return true;

  if (rv == -ENOENT)
    LOG_ERROR("Socket close on read");
  else {
    int err = errno;
    LOG_ERROR("unexpected error: ", rv, " errno: ", err);
  }
  return false;
}

// TODO: Make this thread interruptiple by signal to make it killable if Transport goes away.
void TcpTransport::updateIoStats() {
  do {
    tcpPeers.forall([](std::shared_ptr<TcpPeer> &tp) { tp->updateIoStats(); });
    sleep(1);
  } while (isServing);
}

void TcpTransport::tcpRxThread(unsigned int id) {
  struct epoll_event events[EPOLL_MAXEVENTS]; // NOLINT

  LOG_DEBUG("RX thread ", id, " starting");

  int poll_fd = ::epoll_create1(0);
  if (poll_fd < 0) {
    perror("epoll_create: ");
    return;
  }
  epoll_rfd[id] = poll_fd;

  do {
    int cnt = ::epoll_wait(poll_fd, events, EPOLL_MAXEVENTS, -1);

    for (int i = 0; i < cnt; i++) {
      struct epoll_event *ev = &events[i];
      epoll_epid_t ep_id = {};
      ep_id.data = ev->data.u64;
      int sock = ep_id.id.sock;
      unsigned int epId = ep_id.id.peer_id;
      std::shared_ptr<TcpPeer> tcpPeer = nullptr;

      if (sock < 0) {
        LOG_ERROR("Invalid read socket: ", sock, " PeerId: ", ev->data.u64);
        continue;
      }
      auto it = tcpPeers.get(epId);
      if (it.has_value()) {
        LOG_DEBUG("Found peer for %d: ", sock);
        tcpPeer = *it;
      } else {
        LOG_ERROR("No peer for: ", sock);
        deactivateEndpoint(poll_fd, sock, ALL_CLOSED);
        continue;
      }
      if (ev->events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        deactivateEndpoint(poll_fd, sock, RX_CLOSED);
        if (tcpPeer->SocketStateChange(sock, RX_CLOSED)) {
          tcpPeers.remove(tcpPeer->Id);
        }
        continue;
      }
      if (!(ev->events & EPOLLIN)) {
        LOG_DEBUG("No IN: ", sock);
        continue;
      }

      if (!tcpPeer->processEndpointRecv(sock)) {
        shutdown(sock, SHUT_RDWR);
        deactivateEndpoint(poll_fd, sock, RX_CLOSED);
        if (tcpPeer->SocketStateChange(sock, RX_CLOSED)) {
          tcpPeers.remove(tcpPeer->Id);
        }
      }
    }
  } while (isServing);
  LOG_DEBUG("RX thread ", id, " exiting");
}

std::shared_ptr<TcpTransport> TcpTransport::factory(std::shared_ptr<GEDS> geds) {
  return std::shared_ptr<TcpTransport>(new TcpTransport(geds));
}

void TcpTransport::deactivateEndpoint(int poll_fd, int sock, uint32_t state) {
  if (state & TX_CLOSED)
    epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock, nullptr);
  if (state & RX_CLOSED)
    epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock, nullptr);
}

bool TcpTransport::activateEndpoint(std::shared_ptr<TcpEndpoint> tep,
                                    std::shared_ptr<geds::TcpPeer> peer) {
  struct epoll_event ev = {};
  epoll_epid_t ep_id = {};
  int sock = tep->sock;
  unsigned int thread_id = peer->endpoints.size() % num_proc;

  ep_id.id.sock = tep->sock;
  ep_id.id.peer_id = peer->getId();

  int no = 1;
  if (setsockopt(sock, SOL_TCP, TCP_NODELAY, &no, sizeof(no))) {
    perror("setsockopt nodelay");
    return false;
  }

  ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  ev.data.u64 = ep_id.data;
  if (epoll_ctl(epoll_rfd[thread_id], EPOLL_CTL_ADD, sock, &ev) != 0) {
    perror("epoll_ctl read: ");
    return false;
  }
  ev.events = EPOLLOUT | EPOLLHUP | EPOLLERR | EPOLLET;
  ev.data.u64 = ep_id.data;
  if (epoll_ctl(epoll_wfd[thread_id], EPOLL_CTL_ADD, sock, &ev) != 0) {
    epoll_ctl(epoll_rfd[thread_id], EPOLL_CTL_DEL, sock, NULL);
    perror("epoll_ctl send: ");
    return false;
  }
  LOG_DEBUG("Registered socket ", tep->sock, " for epoll.");
  return true;
}

bool TcpTransport::addEndpointPassive(int sock) {
  std::shared_ptr<TcpEndpoint> tep = std::make_shared<TcpEndpoint>();
  struct sockaddr peer_sockaddr = {};
  auto *in_peer = (sockaddr_in *)&peer_sockaddr;

  socklen_t addrlen = sizeof peer_sockaddr;

  if (::fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK)) {
    perror("fcntl: ");
    return false;
  }

  struct linger lg = {.l_onoff = 0, .l_linger = 0};
  if (::setsockopt(sock, SOL_SOCKET, SO_LINGER, &lg, sizeof lg)) {
    perror("SO_LINGER: ");
    return false;
  }

  if (::getpeername(sock, &peer_sockaddr, &addrlen) != 0) {
    perror("getpeername: ");
    return false;
  }
  tep->sock = sock;

  std::string hostname = inet_ntoa(in_peer->sin_addr);
  std::shared_ptr<TcpPeer> tcpPeer;
  unsigned int epId = SStringHash(hostname);
  auto it = tcpPeers.get(epId);
  if (!it.has_value()) {
    tcpPeer = std::make_shared<TcpPeer>(hostname, _geds, *this);
    tcpPeers.insertOrReplace(epId, tcpPeer);
  } else {
    tcpPeer = *it;
  }
  tcpPeer->addEndpoint(tep);
  activateEndpoint(tep, tcpPeer);
  LOG_DEBUG("Server connected to ", hostname, "::", in_peer->sin_port);

  return true;
}

std::shared_ptr<TcpPeer> TcpTransport::getPeer(sockaddr *peer) {
  auto inaddr = (sockaddr_in *)peer;
  std::string hostname = inet_ntoa(inaddr->sin_addr);
  size_t addrlen = sizeof *peer;
  int sock = -1, rv = 0;
  unsigned int epId = SStringHash(hostname);
  auto lock = getWriteLock();
  /*
   * Check if we are already connected to that address. No new peer in
   * this case.
   */
  std::shared_ptr<TcpPeer> tcpPeer;
  auto it = tcpPeers.get(epId);
  if (it.has_value()) {
    tcpPeer = *it;
    return tcpPeer;
  }
  if (peer->sa_family != AF_INET) {
    LOG_ERROR("Address family not supported: ", peer->sa_family);
    return nullptr;
  }
  for (unsigned int num_ep = 0; num_ep < num_proc; num_ep++) {
    sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      return nullptr;
    }
    rv = ::connect(sock, peer, addrlen);
    if (rv) {
      LOG_DEBUG("cannot connect ", hostname);
      ::close(sock);
      return nullptr;
    }
    /*
     * Mark socket non-blocking to allow efficient handling of
     * multiple sockets in rx and tx threads.
     */
    rv = ::fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
    if (rv) {
      LOG_DEBUG("cannot set socket non-blocking ", hostname);
      close(sock);
      return nullptr;
    }
    struct linger lg = {.l_onoff = 0, .l_linger = 0};
    if (::setsockopt(sock, SOL_SOCKET, SO_LINGER, &lg, sizeof lg)) {
      perror("SO_LINGER: ");
      close(sock);
      return nullptr;
    }
    std::shared_ptr<TcpEndpoint> tep = std::make_shared<TcpEndpoint>();

    LOG_DEBUG("connected ", hostname, "::", inaddr->sin_port);
    tep->sock = sock;
    if (num_ep == 0) {
      tcpPeer = std::make_shared<TcpPeer>(hostname, _geds, *this);
      tcpPeers.insertOrReplace(epId, tcpPeer);
    }
    tcpPeer->addEndpoint(tep);
    activateEndpoint(tep, tcpPeer);
  }
  LOG_DEBUG("Client connected to ", hostname);
  return tcpPeer;
}

void TcpPeer::updateIoStats() {
  epMux.lock_shared();
  for (auto &endpoint : endpoints) {
    auto tep = endpoint.second;
    /*
     * Todo: Implement something more clever here
     */
    tep->tx_bytes /= 2;
    tep->rx_bytes /= 2;
  }
  epMux.unlock_shared();
}

std::shared_ptr<TcpEndpoint> TcpPeer::getLeastUsedTx(size_t to_send) {
  std::shared_ptr<TcpEndpoint> send_tep = nullptr, tep = nullptr;
  size_t min_sent = UINT_LEAST32_MAX;

  epMux.lock_shared();
  for (auto &endpoint : endpoints) {
    tep = endpoint.second;
    if (tep->state != ALL_OPEN) {
      tep = nullptr;
      continue;
    }
    if (tep->send_ctx.state == PROC_IDLE) {
      send_tep = tep;
      break;
    }
    if (tep->tx_bytes + to_send < min_sent) {
      min_sent = tep->tx_bytes;
      send_tep = tep;
    }
  }
  epMux.unlock_shared();
  if (send_tep)
    return send_tep;

  return tep; // May be nullptr
}

int TcpPeer::sendRpcReply(uint64_t reqId, int in_fd, uint64_t start, size_t len, int status) {
  bool send_ok = false;

  auto sendWork = std::make_shared<SocketSendWork>();
  sendWork->reqId = reqId;
  sendWork->va = start;
  sendWork->in_fd = in_fd;
  sendWork->len = len;
  sendWork->type = GET_REPLY;
  sendWork->error = status;
  sendQueue.emplace(sendWork);
  (*sendQueue_stats)++;

  auto tep = getLeastUsedTx(len);
  if (tep) {
    tep->send_ctx.stateMux.lock();
    send_ok = processEndpointSend(tep);
    tep->send_ctx.stateMux.unlock();
  } else {
    LOG_ERROR("No active endpoint found");
  }
  if (send_ok)
    return 0;
  if (errno)
    return -errno;
  return -EIO;
}

std::shared_ptr<std::promise<absl::StatusOr<size_t>>>
TcpPeer::sendRpcRequest(uint64_t dest, std::string name, size_t off, size_t len) {
  uint64_t reqId = ++rpcReqId;
  bool send_ok = false;

  auto recvWork = std::make_shared<SocketRecvWork>();
  recvWork->reqId = reqId;
  recvWork->va = dest;
  recvWork->len = len;
  recvWork->p = std::make_shared<std::promise<absl::StatusOr<size_t>>>();
  recvQueue.insertOrReplace(reqId, recvWork);
  (*recvQueue_stats)++;

  auto sendWork = std::shared_ptr<SocketSendWork>(new SocketSendWork{});
  sendWork->reqId = reqId;
  sendWork->objName = name;
  sendWork->va = 0;
  sendWork->in_fd = -1;
  sendWork->len = len;
  sendWork->off = off;
  sendWork->error = 0;
  sendWork->type = GET_REQ;
  sendQueue.emplace(sendWork);
  (*sendQueue_stats)++;

  auto tep = getLeastUsedTx(len);
  if (tep) {
    tep->send_ctx.stateMux.lock();
    send_ok = processEndpointSend(tep);
    tep->send_ctx.stateMux.unlock();
  } else {
    LOG_ERROR("No active endpoint found");
  }
  if (!send_ok) {
    LOG_ERROR("RPC Req Send failed");
    recvQueue.remove(reqId);
    (*recvQueue_stats)--;
    recvWork->p->set_value(absl::AbortedError("Unable to proceed: "));
  }
  return recvWork->p;
}

uint8_t *TcpTransport::getBuffer() {
  uint8_t *result;
  auto success = _buffers.pop(result);
  if (!success) {
    return new (std::align_val_t(BUFFER_ALIGNMENT)) uint8_t[MIN_SENDFILE_SIZE];
  }
  return result;
}

void TcpTransport::releaseBuffer(uint8_t *buffer) { _buffers.push(buffer); }
} // namespace geds
