/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEDS_GEDS_H
#define GEDS_GEDS_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <utility>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <boost/asio/thread_pool.hpp>

#include "ConcurrentMap.h"
#include "ConcurrentSet.h"
#include "FileTransferService.h"
#include "GEDSConfig.h"
#include "GEDSFileHandle.h"
#include "GEDSFileStatus.h"
#include "GEDSInternal.h"
#include "GEDSLocalFileHandle.h"
#include "HttpServer.h"
#include "MetadataService.h"
#include "Object.h"
#include "ObjectStoreConfig.h"
#include "Path.h"
#include "RWConcurrentObjectAdaptor.h"
#include "S3Endpoint.h"
#include "S3ObjectStores.h"
#include "Server.h"
#include "Statistics.h"
#include "TcpTransport.h"

const char Default_GEDSFolderDelimiter = '/';

class GEDSFile;

class GEDS : public std::enable_shared_from_this<GEDS>, utility::RWConcurrentObjectAdaptor {
  GEDSConfig _config;

public:
  const GEDSConfig &config() const { return _config; }

protected:
  /**
   * @brief GEDS Server instance that allows file transfers.
   */
  geds::Server _server;

  /**
   * @brief GEDS Service state.
   */
  geds::ServiceState _state{geds::ServiceState::Stopped};

  /**
   * @brief Metadata service.
   */
  geds::MetadataService _metadataService;

  /**
   * @brief GEDS local directory path. Folder/path-prefix which stores all local GEDS data.
   *
   */
  const std::string _pathPrefix;
  mutable std::atomic<size_t> _fileNameCounter;
  mutable utility::ConcurrentMap<std::string, size_t> _fileNames;

  /**
   * @brief URI for Local Host.
   */
  std::string _hostURI;
  std::string _hostname;

  /**
   * @brief Filehandles known to the local GEDS instance.
   */
  utility::ConcurrentMap<utility::Path, std::shared_ptr<GEDSFileHandle>, std::less<>> _fileHandles;
  inline utility::Path getPath(const std::string &bucket, const std::string &key) {
    return {bucket + "/" + key};
  }
  utility::ConcurrentMap<std::string, std::shared_ptr<geds::FileTransferService>> _fileTransfers;

  utility::ConcurrentSet<std::string> _knownBuckets;

  geds::s3::ObjectStores _objectStores;

  std::shared_ptr<geds::StatisticsCounter> _statisticsFilesOpened =
      geds::Statistics::createCounter("GEDS: files opened");
  std::shared_ptr<geds::StatisticsCounter> _statisticsFilesCreated =
      geds::Statistics::createCounter("GEDS: files created");

  geds::HttpServer _httpServer;

  boost::asio::thread_pool _ioThreadPool;
  std::thread _storageMonitoringThread;
  void startStorageMonitoringThread();

  std::atomic<size_t> _localStorageUsed;
  std::atomic<size_t> _localMemoryUsed;

public:
  /**
   * @brief GEDS CTOR. Note: This CTOR needs to be wrapped in a SHARED_POINTER!
   */
  GEDS(GEDSConfig &&argConfig);

  /**
   * @brief Constructor wrapper which forces a shared_ptr.
   */
  [[nodiscard]] static std::shared_ptr<GEDS> factory(GEDSConfig config);

  virtual ~GEDS();

  /**
   * @brief Start GEDS.
   */
  absl::Status start();

  /**
   * @brief Stop GEDS.
   */
  absl::Status stop();

  /**
   * @brief Check if the bucket name is allowed.
   * In order to ensure compatibility with S3 we make sure that a bucket name
   * - Consists of only lower case ASCII characers, numbers, dots and hypens.
   * - The bucket name must begin and end with a letter or number.
   * - The bucket must be at least 3 characters long.
   */
  static absl::Status isValidBucketName(const std::string &bucket);

  /**
   * @brief Validate the name of the key.
   * - Keys shall not start with `/`.
   * - A length of zero is not allowed.
   * - Keys are not allowed to end with `/`.
   * - `/../` as part of a key is not allowed.
   */
  static absl::Status isValidKeyName(const std::string &key);

  /**
   * @brief Validate the name of bucket and key.
   */
  static absl::Status isValid(const std::string &bucket, const std::string &key);

  /**
   * @brief Parse the object name and split it into bucket and key.
   */
  static absl::StatusOr<std::pair<std::string, std::string>> parseObjectName(const std::string& objectName);

  /**
   * @brief Create object located at bucket/key.
   * The object is registered with the metadata service once the file is sealed.
   */
  absl::StatusOr<GEDSFile> create(const std::string &objectName, bool overwrite = false);
  absl::StatusOr<GEDSFile> create(const std::string &bucket, const std::string &key,
                                  bool overwrite = false);
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  createAsFileHandle(const std::string &bucket, const std::string &key, bool overwrite = false);

  /**
   * @brief Recursively create directory using directory markers.
   */
  absl::Status mkdirs(const std::string &bucket, const std::string &path,
                      char delimiter = Default_GEDSFolderDelimiter);

  /**
   * @brief Register a bucket with the metadata server.
   */
  absl::Status createBucket(const std::string &bucket);

  absl::Status lookupBucket(const std::string &bucket);

  /**
   * @brief Open object located at bucket/key.
   */
  absl::StatusOr<GEDSFile> open(const std::string &objectName);
  absl::StatusOr<GEDSFile> open(const std::string &bucket, const std::string &key,
                                bool retry = true);
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> openAsFileHandle(const std::string &bucket,
                                                                   const std::string &key);
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
  reopenFileHandle(const std::string &bucket, const std::string &key, bool invalidate);

  /**
   * @brief Reopen a file after a unsuccessful read.
   */
  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> reopen(std::shared_ptr<GEDSFileHandle> existing);

  /**
   * @brief Only open local filehandles.
   */
  absl::StatusOr<GEDSFile> localOpen(const std::string &objectName);
  absl::StatusOr<GEDSFile> localOpen(const std::string &bucket, const std::string &key);

  /**
   * @brief Mark the file associated with fileHandle as sealed.
   */
  absl::Status seal(GEDSFileHandle &fileHandle, bool update, size_t size,
                    std::optional<std::string> uri = std::nullopt);

  /**
   * @brief List objects in bucket where the key starts with `prefix`.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> list(const std::string &bucket,
                                                   const std::string &prefix);

  /**
   * @brief List objects in `bucket` where the key starts with `prefix` and the postfix does not
   * contain `delimiter`.
   * - If the delimiter set to `0` will list all keys starting with prefix.
   * - If the delimiter is set to a value != 0, then the delimiter will be used as a folder
   * separator. Keys ending with "/_$folder$" will be used as directory markers (where '/' is used
   * as a delimiter).
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> list(const std::string &bucket,
                                                   const std::string &prefix, char delimiter);

  /**
   * @brief List objects from cache in `bucket` where the key starts with `prefix` and the postfix does not
   * contain `delimiter`.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> listFromCache(const std::string &bucket,
                                                   const std::string &prefix, char delimiter, const bool useCache);

  /**
   * @brief List objects in `bucket` with `/` acting as delimiter.
   */
  absl::StatusOr<std::vector<GEDSFileStatus>> listAsFolder(const std::string &bucket,
                                                           const std::string &prefix);

  /**
   * @brief Get status of `key` in `bucket`.
   */
  absl::StatusOr<GEDSFileStatus> status(const std::string &bucket, const std::string &key);
  absl::StatusOr<GEDSFileStatus> status(const std::string &bucket, const std::string &key,
                                        char delimiter);

  /**
   * @brief Rename a prefix recursively.
   */
  absl::Status renamePrefix(const std::string &bucket, const std::string &srcPrefix,
                            const std::string &destPrefix);
  absl::Status renamePrefix(const std::string &srcBucket, const std::string &srcPrefix,
                            const std::string &destPrefix, const std::string &destKey);

  /**
   * @brief Rename an object.
   */
  absl::Status rename(const std::string &bucket, const std::string &srcKey,
                      const std::string &destKey);
  absl::Status rename(const std::string &srcBucket, const std::string &srcKey,
                      const std::string &destBucket, const std::string &destKey);

  /**
   * @brief Copy a file or a folder structure.
   */
  absl::Status copyPrefix(const std::string &bucket, const std::string &srcPrefix,
                          const std::string &destPrefix);
  absl::Status copyPrefix(const std::string &srcBucket, const std::string &srcPrefix,
                          const std::string &destPrefix, const std::string &destKey);

  /**
   * @brief Copy an object.
   */
  absl::Status copy(const std::string &bucket, const std::string &srcKey,
                    const std::string &destKey);
  absl::Status copy(const std::string &srcBucket, const std::string &srcKey,
                    const std::string &destBucket, const std::string &destKey);

  /**
   * @brief Delete object in `bucket` with `key`.
   * @returns absl::OkStatus if the operation has been successful or not objects have been deleted.
   */
  absl::Status deleteObject(const std::string &bucket, const std::string &key);

  /**
   * @brief Delete objects in `bucket` with keys starting with `prefix`.
   * @returns absl::OkStatus if the operation has been successful or not objects have been deleted.
   */
  absl::Status deleteObjectPrefix(const std::string &bucket, const std::string &prefix);

  /**
   * @brief Compute the path to the files stored in `_pathPrefix` folder.
   */
  std::string getLocalPath(const std::string &bucket, const std::string &key) const;
  std::string getLocalPath(const GEDSFile &file) const;

  /**
   * @brief Tcp inter-node object transport service.
   *
   */
  std::shared_ptr<geds::TcpTransport> _tcpTransport;

  /**
   * @brief Register an object store configuration with GEDS.
   */
  absl::Status registerObjectStoreConfig(const std::string &bucket, const std::string &endpointUrl,
                                         const std::string &accessKey,
                                         const std::string &secretKey);

  /**
   * @brief Sync object store configs.
   */
  absl::Status syncObjectStoreConfigs();

  absl::StatusOr<std::shared_ptr<geds::s3::Endpoint>> getS3Endpoint(const std::string &s3Bucket);
  absl::StatusOr<std::shared_ptr<geds::FileTransferService>>
  getFileTransferService(const std::string &hostname);

  void relocate(bool force = false);
  void relocate(std::vector<std::shared_ptr<GEDSFileHandle>> &relocatable, bool force = false);
  void relocate(std::shared_ptr<GEDSFileHandle> handle, bool force = false);

  size_t localStorageUsed() const;
  size_t localStorageFree() const;
  size_t localStorageAllocated() const;
  size_t localMemoryUsed() const;
  size_t localMemoryFree() const;
  size_t localMemoryAllocated() const;

  absl::Status subscribeStreamWithThread(const geds::SubscriptionEvent &event);
  absl::Status stopSubscribeStreamWithThread();
  absl::Status subscribeStream(const geds::SubscriptionEvent &event);
  absl::Status subscribe(const geds::SubscriptionEvent &event);
  absl::Status unsubscribe(const geds::SubscriptionEvent &event);
};

#endif // GEDS_GEDS_H
