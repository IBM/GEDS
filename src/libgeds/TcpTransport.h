/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TCP_TRANSPORT_H
#define _TCP_TRANSPORT_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <shared_mutex>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <utility>

#include <absl/status/statusor.h>
#include <boost/lockfree/stack.hpp>

#include "ConcurrentMap.h"
#include "ConcurrentQueue.h"
#include "FileTransferProtocol.h"
#include "RWConcurrentObjectAdaptor.h"
#include "Statistics.h"
#include "StatisticsGauge.h"

class GEDS;

namespace geds {

enum SockProcessingState { PROC_IDLE = 0, PROC_HDR, PROC_DATA, PROC_FAILED };

enum TcpRpcOp { GET_REQ = 1, GET_REPLY, INFO_REQ, INFO_REPLY };

/**
 * @brief Work item for threads sending and receiving on TCP socket
 *
 */
struct SocketSendWork {
  std::string objName;
  uint64_t reqId;
  uint64_t va;
  int in_fd;
  size_t off;
  size_t len;
  TcpRpcOp type;
  size_t progress;
  int error;
};

struct SocketRecvWork {
  uint64_t reqId;
  uint64_t va;
  size_t len;
  std::shared_ptr<std::promise<absl::StatusOr<size_t>>> p;
};

/**
 * @brief Hdr of all TCP RPC's
 *
 */
struct TcpCtlHdr {
  uint64_t reqid;
  uint64_t datalen;
  uint64_t offset;
  uint16_t hdrlen;
  uint8_t type;
  uint8_t error; // just errors as defined in errno.h
  uint32_t pad;
};

// 4096 bytes maximum object name length
#define RPC_TCP_MAX_HDR (4096 + sizeof(TcpCtlHdr))

struct TcpSendState {
  std::atomic<SockProcessingState> state = PROC_IDLE;
  mutable std::shared_mutex stateMux;
  bool direct_tx = false;
  struct TcpCtlHdr hdr;
  uint64_t va;
  int in_fd; // If non-negative: fd of requested object, to be used in sendfile()
  std::string objName;
  size_t progress;
};

struct TcpRcvState {
  std::atomic<SockProcessingState> state = PROC_IDLE;
  struct TcpCtlHdr hdr;
  uint64_t va;
  std::string objName;
  size_t progress;

  std::shared_ptr<std::promise<absl::StatusOr<size_t>>> p;
};

class TcpPeer;

enum epState {
  ALL_OPEN = 0x0,
  TX_CLOSED = 0x01,
  RX_CLOSED = 0x02,
  ALL_CLOSED = TX_CLOSED | RX_CLOSED
};

struct TcpEndpoint {
  int sock;
  uint32_t state;
  struct TcpRcvState recv_ctx;
  struct TcpSendState send_ctx;
  size_t tx_bytes = 0;
  size_t rx_bytes = 0;
};

/**
 * @brief simple hash function to hash peer name to list index
 *
 * @param name
 * @return unsigned int
 */
static unsigned int SStringHash(std::string name) {
  unsigned int hash = 0;
  for (char i : name)
    hash = i + (hash << 6) + (hash << 16) - hash;
  return hash;
}

struct ep_id {
  uint32_t peer_id;
  int32_t sock;
};

using epoll_epid_t = union EpollEpId {
  u_int64_t data;
  struct ep_id id;
};

class TcpTransport;
class TcpPeer : public std::enable_shared_from_this<TcpPeer>, utility::RWConcurrentObjectAdaptor {

private:
  friend class TcpTransport;

  unsigned int Id;
  std::shared_ptr<GEDS> _geds;
  TcpTransport &_tcpTransport;
  std::string hostname;
  std::atomic_uint64_t rpcReqId = 0;

  utility::ConcurrentQueue<std::shared_ptr<SocketSendWork>> sendQueue;
  std::shared_ptr<StatisticsGauge> sendQueue_stats =
      Statistics::createGauge("GEDS: TcpTransport sendQueue length");
  utility::ConcurrentMap<uint64_t, std::shared_ptr<SocketRecvWork>> recvQueue;
  std::shared_ptr<StatisticsGauge> recvQueue_stats =
      Statistics::createGauge("GEDS: TcpTransport recvQueue length");

  std::map<int, std::shared_ptr<TcpEndpoint>> endpoints;

  bool processEndpointSend(std::shared_ptr<TcpEndpoint> tep);
  bool processEndpointRecv(int sock);
  bool SocketStateChange(int sock, uint32_t change);
  bool SocketTxReady(int sock);
  void updateIoStats();
  void cleanup();

  std::shared_ptr<TcpEndpoint> getLeastUsedTx(size_t tu_send);

  int sendRpcReply(uint64_t reqId, uint64_t start, size_t len, int status);
  void TcpProcessRpcGet(uint64_t ReqId, const std::string ObjName, size_t len, size_t off);

public:
  unsigned int getId() { return Id; }
  std::shared_ptr<std::promise<absl::StatusOr<size_t>>>
  sendRpcRequest(uint64_t dest, std::string name, size_t src_off, size_t len);
  int sendRpcReply(uint64_t reqId, int in_fd, uint64_t start, size_t len, int status);
  void addEndpoint(std::shared_ptr<TcpEndpoint> tep) {
    auto lock = getWriteLock();
    endpoints.emplace(tep->sock, tep);
  };
  TcpPeer(std::string name, std::shared_ptr<GEDS> geds, TcpTransport &tcpTransport)
      : Id(SStringHash(name)), _geds(std::move(geds)), _tcpTransport(tcpTransport),
        hostname(std::move(name)){};
  TcpPeer(const TcpPeer &other) = delete;
  TcpPeer(TcpPeer &&other) = delete;
  TcpPeer &operator=(const TcpPeer &other) = delete;
  TcpPeer &operator=(TcpPeer &&other) = delete;
  ~TcpPeer();
};
constexpr unsigned int MAX_PEERS = 8096;
constexpr unsigned int MAX_IO_THREADS = 8;
constexpr unsigned int EPOLL_MAXEVENTS = MAX_PEERS / MAX_IO_THREADS;

class TcpTransport : public std::enable_shared_from_this<TcpTransport> {

private:
  std::shared_ptr<GEDS> _geds;
  boost::lockfree::stack<uint8_t *, boost::lockfree::fixed_sized<false>> _buffers{MAX_IO_THREADS};

  void tcpTxThread(unsigned int id);
  void tcpRxThread(unsigned int id);
  std::vector<std::unique_ptr<std::thread>> txThreads;
  std::vector<std::unique_ptr<std::thread>> rxThreads;

  void updateIoStats();
  std::unique_ptr<std::thread> ioStatsThread;

  volatile bool isServing = false;
  unsigned int num_proc = 0;

  int epoll_rfd[MAX_IO_THREADS] = {}; // for epoll() receive
  int epoll_wfd[MAX_IO_THREADS] = {}; // for epoll() send

  void deactivateEndpoint(int poll_fd, int sock, uint32_t state);
  bool activateEndpoint(std::shared_ptr<TcpEndpoint>, std::shared_ptr<TcpPeer>);
  utility::ConcurrentMap<unsigned int, std::shared_ptr<TcpPeer>> tcpPeers;

  mutable std::shared_mutex connMutex;
  auto getReadLock() const { return std::shared_lock<std::shared_mutex>(connMutex); }
  auto getWriteLock() const { return std::unique_lock<std::shared_mutex>(connMutex); }

  TcpTransport(std::shared_ptr<GEDS> geds);

public:
  [[nodiscard]] static std::shared_ptr<TcpTransport> factory(std::shared_ptr<GEDS> geds);

  uint8_t *getBuffer();
  void releaseBuffer(uint8_t *buffer);

  virtual ~TcpTransport();
  TcpTransport(const TcpTransport &other) = delete;
  TcpTransport(TcpTransport &&other) = delete;
  TcpTransport &operator=(const TcpTransport &other) = delete;
  TcpTransport &operator=(TcpTransport &&other) = delete;

  void start();
  void stop();

  std::shared_ptr<TcpPeer> getPeer(sockaddr *);
  bool addEndpointPassive(int sock);
};
} // namespace geds
#endif
