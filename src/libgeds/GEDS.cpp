/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GEDS.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <typeinfo>
#include <utility>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <absl/strings/escaping.h>

#include "DirectoryMarker.h"
#include "FileTransferService.h"
#include "Filesystem.h"
#include "GEDSCachedFileHandle.h"
#include "GEDSConfig.h"
#include "GEDSFile.h"
#include "GEDSFileHandle.h"
#include "GEDSFileStatus.h"
#include "GEDSInternal.h"
#include "GEDSLocalFileHandle.h"
#include "GEDSMMapFileHandle.h"
#include "GEDSRemoteFileHandle.h"
#include "GEDSS3FileHandle.h"
#include "Logging.h"
#include "Object.h"
#include "Path.h"
#include "Platform.h"
#include "Ports.h"
#include "Statistics.h"
#include "TcpTransport.h"
#include "Version.h"

using namespace geds;

static std::string computeHostUri(const std::string &hostname, uint16_t port) {
  return "geds://" + hostname + ":" + std::to_string(port);
}

GEDS::GEDS(GEDSConfig &&argConfig)
    : std::enable_shared_from_this<GEDS>(), _config(argConfig),
      _server(_config.listenAddress, _config.port),
      _metadataService(_config.metadataServiceAddress), _pathPrefix(_config.localStoragePath),
      _hostname(_config.hostname.value_or("")), _httpServer(_config.portHttpServer) {
  std::error_code ec;
  auto success = std::filesystem::create_directories(_pathPrefix, ec);
  if (!success && ec.value() != 0) {
    auto message = "Unable to create prefix " + _pathPrefix + ". Reason " + ec.message();
    LOG_ERROR(message);
    throw std::runtime_error(message);
  }
}

static std::string computeLocalStoragePath(std::string path) {
  if (path.ends_with("XXXXXX")) {
    return geds::filesystem::mktempdir(path);
  }
  return path;
}

std::shared_ptr<GEDS> GEDS::factory(GEDSConfig config) {
  config.localStoragePath = computeLocalStoragePath(config.localStoragePath);
  // Call private CTOR.
  return std::make_shared<GEDS>(std::move(config));
}

GEDS::~GEDS() {
  LOG_DEBUG("GEDS Destructor. state: ", (int)_state);
  if (_state == ServiceState::Running) {
    LOG_INFO("Stopping GEDS Service");
    (void)stop();
  }
}

#define GEDS_CHECK_SERVICE_RUNNING                                                                 \
  if (_state != ServiceState::Running) {                                                           \
    return absl::FailedPreconditionError("The service is " + to_string(_state) + ".");             \
  }

absl::Status GEDS::start() {
  std::cout << "Starting GEDS (" << utility::GEDSVersion() << ")\n"
            << "- prefix: " << _pathPrefix << "\n"
            << "- metadata service: " << _metadataService.serverAddress << std::endl;
  if (_state != ServiceState::Stopped) {
    return absl::FailedPreconditionError("The service is " + to_string(_state) + ".");
  }
  _state = ServiceState::Unknown;
  // Connect to metadata service.
  auto result = _metadataService.connect();
  if (!result.ok()) {
    return result;
  }
  // Start GEDS Service.
  result = _server.start(shared_from_this());
  if (!result.ok()) {
    return result;
  }

  if (!_hostname.empty()) {
    _hostURI = computeHostUri(_hostname, _server.port());
    LOG_INFO("Hostname was declared, using ", _hostURI, " to announce myself.");
  } else {
    auto hostIP = _metadataService.getConnectionInformation();
    if (!hostIP.ok()) {
      return absl::UnknownError("Unable to obtain connection IP! Reason: " +
                                std::string{hostIP.status().message()});
    }
    _hostURI = computeHostUri(hostIP.value(), _server.port());
    LOG_INFO("Using ", _hostURI, " to announce myself.");
  }

  _tcpTransport = TcpTransport::factory(shared_from_this());
  _tcpTransport->start();

  //  result = _httpServer.start();
  //  if (!result.ok()) {
  //    LOG_ERROR("Unable to start webserver.");
  //  }

  // Update state.
  _state = ServiceState::Running;

  auto st = syncObjectStoreConfigs();
  if (!syncObjectStoreConfigs().ok()) {
    LOG_ERROR("Unable to synchronize object store configs on boot.");
  }

  return absl::OkStatus();
}

absl::Status GEDS::subscribeStreamWithThread(const geds::SubscriptionEvent &event) {
  GEDS_CHECK_SERVICE_RUNNING

  std::thread subscriberTread(&GEDS::subscribeStream, this, event);
  subscriberTread.detach();

  return absl::OkStatus();
}

absl::Status GEDS::subscribeStream(const geds::SubscriptionEvent &event) {
  GEDS_CHECK_SERVICE_RUNNING
  return _metadataService.subscribeStream(event);
}

absl::Status GEDS::subscribe(const geds::SubscriptionEvent &event) {
  GEDS_CHECK_SERVICE_RUNNING
  return _metadataService.subscribe(event);
}

absl::Status GEDS::unsubscribe(const geds::SubscriptionEvent &event) {
  GEDS_CHECK_SERVICE_RUNNING
  return _metadataService.unsubscribe(event);
}

absl::Status GEDS::stop() {
  GEDS_CHECK_SERVICE_RUNNING
  LOG_INFO("Stopping");
  LOG_INFO("Printing statistics");

  geds::Statistics::print();
  auto result = _metadataService.disconnect();
  if (!result.ok()) {
    LOG_ERROR("cannot disconnect metadata service");
    _state = ServiceState::Unknown;
    return result;
  }
  result = _server.stop();
  if (!result.ok()) {
    _state = ServiceState::Unknown;
    LOG_ERROR("cannot stop server");
    return result;
  }

  _httpServer.stop();

  // XXX TODO: Properly cleanup files
  _fileHandles.clear();
  _fileTransfers.clear();
  _tcpTransport->stop();

  _state = ServiceState::Stopped;
  return result;
}

absl::Status GEDS::isValidBucketName(const std::string &bucket) {
  // Loosely follow the S3 bucket naming rules.
  static std::regex regex("[a-z\\d][a-z\\d\\.\\-]+[a-z\\d]", std::regex_constants::ECMAScript);
  if (std::regex_match(bucket, regex)) {
    if (bucket.size() >= 4 && bucket.substr(0, 4).compare("xn--") == 0) {
      return absl::FailedPreconditionError("Invalid bucket name.");
    }
    return absl::OkStatus();
  }
  return absl::FailedPreconditionError("Invalid bucket name.");
}

absl::Status GEDS::isValidKeyName(const std::string &key) {
  if (key.size() == 0) {
    return absl::FailedPreconditionError("Zero-length keys are not allowed.");
  }
  if (key.back() == '/') {
    return absl::FailedPreconditionError("Keys are not allowed to end with \"/\".");
  }
  if (key.size() >= 2 && key.substr(0, 1).compare("./") == 0) {
    return absl::FailedPreconditionError("Keys are not allowed to start with \"./\".");
  }
  if (key.size() >= 3 && key.substr(0, 3).compare("../") == 0) {
    return absl::FailedPreconditionError("Keys are not allowed to start with \"../\".");
  }
  if (key.find("/../") != std::string::npos) {
    return absl::FailedPreconditionError("Keys are not allowed to contain \"/../\"");
  }
  return absl::OkStatus();
}

absl::Status GEDS::isValid(const std::string &bucket, const std::string &key) {
  auto result = isValidBucketName(bucket);
  if (!result.ok()) {
    return result;
  }
  result = isValidKeyName(key);
  if (!result.ok()) {
    return result;
  }
  return absl::OkStatus();
}

absl::StatusOr<GEDSFile> GEDS::create(const std::string &bucket, const std::string &key,
                                      bool overwrite) {
  LOG_DEBUG("create ", bucket, "/", key);
  auto result = createAsFileHandle(bucket, key, overwrite);
  if (result.ok()) {
    *_statisticsFilesCreated += 1;
    return (*result)->open();
  }
  return result.status();
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
GEDS::createAsFileHandle(const std::string &bucket, const std::string &key, bool overwrite) {
  GEDS_CHECK_SERVICE_RUNNING

  const auto check = GEDS::isValid(bucket, key);
  if (!check.ok()) {
    return check;
  }
  auto bucketStatus = createBucket(bucket);
  if (!bucketStatus.ok()) {
    return bucketStatus;
  }

  const auto path = getPath(bucket, key);
  auto handle = GEDSLocalFileHandle::factory(shared_from_this(), bucket, key);
  if (!handle.ok()) {
    return handle.status();
  }

  if (overwrite) {
    _fileHandles.insertOrReplace(path, *handle);
    return *handle;
  }
  auto newHandle = _fileHandles.insertOrExists(path, *handle);
  if (newHandle.get() != handle->get()) {
    return absl::AlreadyExistsError("The file " + path.name + "already exists!");
  }
  return newHandle;
}

absl::Status GEDS::mkdirs(const std::string &bucket, const std::string &path, char delimiter) {
  if (path.empty() || (path.size() == 1 && path[0] == delimiter)) {
    return absl::OkStatus();
  }
  if (path.back() != delimiter) {
    return mkdirs(bucket, path + delimiter);
  }
  auto folderPath = path + Default_DirectoryMarker;
  LOG_DEBUG("Creating ", bucket, "/", folderPath);
  auto mkdir = create(bucket, folderPath);
  if (!mkdir.ok() && mkdir.status().code() != absl::StatusCode::kAlreadyExists) {
    LOG_ERROR("Unable to create folder ", folderPath);
    return mkdir.status();
  }
  if (path.size() >= 2) {
    auto stripPos = path.substr(0, path.size() - 1).rfind(delimiter);
    if (stripPos == std::string::npos) {
      return absl::OkStatus();
    }
    return mkdirs(bucket, path.substr(0, stripPos));
  }
  return absl::OkStatus();
}

absl::Status GEDS::createBucket(const std::string &bucket) {
  auto status = GEDS::isValidBucketName(bucket);
  if (!status.ok()) {
    return status;
  }
  if (lookupBucket(bucket).ok()) {
    return absl::OkStatus();
  }
  status = _metadataService.createBucket(bucket);
  // Allow multiple creations of the same bucket.
  if (status.ok() || status.code() == absl::StatusCode::kAlreadyExists) {
    return absl::OkStatus();
  }
  return status;
}

absl::Status GEDS::lookupBucket(const std::string &bucket) {
  if (_knownBuckets.exists(bucket)) {
    return absl::OkStatus();
  }
  auto status = _metadataService.lookupBucket(bucket);
  if (!status.ok()) {
    return status;
  }
  _knownBuckets.insert(bucket);
  return absl::OkStatus();
}

absl::StatusOr<GEDSFile> GEDS::open(const std::string &bucket, const std::string &key) {
  LOG_DEBUG("open ", bucket, "/", key);
  auto fh = openAsFileHandle(bucket, key);
  if (fh.ok()) {
    *_statisticsFilesOpened += 1;
    return (*fh)->open();
  }
  return fh.status();
}

absl::StatusOr<std::shared_ptr<GEDSFileHandle>>
GEDS::openAsFileHandle(const std::string &bucket, const std::string &key, bool invalidate) {
  const auto path = getPath(bucket, key);

  GEDS_CHECK_SERVICE_RUNNING
  auto check = GEDS::isValid(bucket, key);
  if (!check.ok()) {
    return check;
  }

  // Check if file is already open on the machine.
  {
    auto fileHandle = _fileHandles.get(path);
    if (fileHandle.has_value()) {
      return (*fileHandle);
    }
  }

  // File is not available locally. Lookup in metadata service instead.
  auto status_file = _metadataService.lookup(bucket, key, invalidate);
  if (!status_file.ok()) {
    // The file is not registered.
    // Try open file on s3:
    auto s3FileHandle =
        GEDSCachedFileHandle::factory<GEDSS3FileHandle>(shared_from_this(), bucket, key);
    if (s3FileHandle.ok()) {
      // Use file handle registered with fileHandles object.
      auto eexists = _fileHandles.insertOrExists(path, *s3FileHandle);
      if (s3FileHandle->get() == eexists.get()) {
        // Seal filehandle to avoid extranous lookups.
        (void)eexists->seal();
      }
      return eexists;
    }
    // The file does not exist.
    // Otherwise return not found from metadata service.
    if (s3FileHandle.status().code() != absl::StatusCode::kNotFound) {
      return s3FileHandle.status();
    }
    return status_file.status();
  }

  const auto &object = status_file.value();
  const auto &location = object.info.location;

  const std::string_view s3Prefix{"s3://"};
  const std::string_view gedsPrefix{"geds://"};

  absl::StatusOr<std::shared_ptr<GEDSFileHandle>> fileHandle;
  if (location.compare(0, gedsPrefix.size(), gedsPrefix) == 0) {
    fileHandle = GEDSRemoteFileHandle::factory(shared_from_this(), object);
  } else if (location.compare(0, s3Prefix.size(), s3Prefix) == 0) {
    fileHandle = GEDSCachedFileHandle::factory<GEDSS3FileHandle>(shared_from_this(), object);
  } else {
    return absl::UnknownError("The remote location format " + location + " is not known.");
  }

  if (!fileHandle.ok()) {
    if (!invalidate) {
      return openAsFileHandle(bucket, key, true);
    }
    // Delete object if the file does not exist.
    if (fileHandle.status().code() == absl::StatusCode::kNotFound) {
      LOG_INFO("Deleting ", bucket, "/", key, " associated with ", object.info.location,
               " since it does not exist.");
      (void)deleteObject(bucket, key);
    }
    LOG_ERROR("Unable to open ", bucket, "/", key, " reason: ", fileHandle.status().message());
    return fileHandle.status();
  }
  // Prevent race condition.
  return _fileHandles.insertOrExists(path, *fileHandle);
}

absl::StatusOr<std::shared_ptr<geds::s3::Endpoint>> GEDS::getS3Endpoint(const std::string &bucket) {
  return _objectStores.get(bucket);
}

absl::StatusOr<std::shared_ptr<geds::FileTransferService>>
GEDS::getFileTransferService(const std::string &hostname) {
  {
    auto fileTransferService = _fileTransfers.get(hostname);
    if (fileTransferService.has_value()) {
      return *fileTransferService;
    }
  }
  auto fileTransferService =
      std::make_shared<FileTransferService>(hostname, shared_from_this(), _tcpTransport);
  auto status = fileTransferService->connect();
  if (!status.ok()) {
    LOG_ERROR("Unable to connect to ", hostname, " for FileTransferService.: ", status.code());
    return absl::UnavailableError("Unable to connect to " + hostname + ": " +
                                  std::string{status.message()});
  }
  // Insert file transfer service.
  return _fileTransfers.insertOrExists(hostname, fileTransferService);
}

absl::Status GEDS::seal(GEDSFileHandle &fileHandle, bool update, size_t size,
                        std::optional<std::string> uri) {
  GEDS_CHECK_SERVICE_RUNNING
  auto obj = geds::Object{geds::ObjectID{fileHandle.bucket, fileHandle.key},
                          geds::ObjectInfo{uri.value_or(_hostURI), size, size}};
  if (update) {
    return _metadataService.updateObject(obj);
  }
  return _metadataService.createObject(obj);
}

std::string GEDS::getLocalPath(const std::string &bucket, const std::string &key) const {
  auto postfix = bucket + "/" + key;
  auto exists = _fileNames.get(postfix);
  if (exists.has_value()) {
    return _pathPrefix + "/" + bucket + "/" + std::to_string(*exists);
  }
  auto value = _fileNameCounter++;
  auto n2 = _fileNames.insertOrExists(key, value);
  return _pathPrefix + "/" + bucket + "/" + std::to_string(n2);
}

std::string GEDS::getLocalPath(const GEDSFile &file) const {
  return getLocalPath(file.bucket(), file.key());
}

absl::StatusOr<std::vector<GEDSFileStatus>> GEDS::list(const std::string &bucket,
                                                       const std::string &prefix) {
  return list(bucket, prefix, 0);
}

absl::StatusOr<std::vector<GEDSFileStatus>> GEDS::list(const std::string &bucket,
                                                       const std::string &prefix, char delimiter) {
  bool prefixExists = false;
  auto list = _metadataService.listPrefix(bucket, prefix, delimiter);
  if (!list.ok()) {
    return list.status();
  }
  _knownBuckets.insert(bucket);
  const std::string folderString = delimiter + Default_DirectoryMarker;
  std::set<GEDSFileStatus> result;
  for (const auto &value : list->first) {
    prefixExists = true;
    const auto &key = value.id.key;
    if (delimiter != 0 && key.ends_with(folderString)) {
      // Don't list current directory.
      continue;
    } else {
      result.emplace(GEDSFileStatus{.key = key, .size = value.info.size, .isDirectory = false});
    }
  }
  for (const auto &prefix : list->second) {
    result.emplace(GEDSFileStatus{.key = prefix, .size = 0, .isDirectory = true});
  }
  auto extStatus = _objectStores.get(bucket);
  if (extStatus.ok()) {
    auto ext = extStatus.value();
    auto s3status = ext->list(bucket, prefix, delimiter, result);
    if (s3status.ok()) {
      prefixExists = true;
    } else {
      return s3status;
    }
  }
  if (result.empty() && delimiter && !prefixExists) {
    return absl::NotFoundError("Prefix not found: " + prefix);
  }
  return std::vector<GEDSFileStatus>{result.begin(), result.end()};
}

absl::StatusOr<std::vector<GEDSFileStatus>> GEDS::listAsFolder(const std::string &bucket,
                                                               const std::string &prefix) {
  return list(bucket, prefix, Default_GEDSFolderDelimiter);
}

absl::StatusOr<GEDSFileStatus> GEDS::status(const std::string &bucket, const std::string &key) {
  return status(bucket, key, Default_GEDSFolderDelimiter);
}

absl::StatusOr<GEDSFileStatus> GEDS::status(const std::string &bucket, const std::string &key,
                                            char delimiter) {
  // Base case: Empty key, or key matching `/`.
  if (key.size() == 0 || (key.size() == 1 && key[0] == delimiter)) {
    auto isRegistered = lookupBucket(bucket);
    if (isRegistered.ok() || _objectStores.get(bucket).ok()) {
      return GEDSFileStatus{.key = key, .size = 0, .isDirectory = true};
    }
    return absl::NotFoundError("Bucket not found!");
  }

  auto listDir = [&](const std::string &k) -> absl::StatusOr<GEDSFileStatus> {
    auto list = _metadataService.listPrefix(bucket, k, delimiter);
    if (list.ok() && list->second.size() > 0) {
      return GEDSFileStatus{.key = k, .size = 0, .isDirectory = true};
    }
    auto s3 = _objectStores.get(bucket);
    if (s3.ok()) {
      auto s3Status = (*s3)->folderStatus(bucket, k, delimiter);
      if (s3Status.ok() || s3Status.status().code() != absl::StatusCode::kNotFound) {
        return s3Status;
      }
    }
    return absl::NotFoundError("Key " + key + " not found!");
  };

  // Location is a directory.
  auto isDir = key.back() == delimiter;
  if (isDir) {
    return listDir(key);
  }

  // Location is most likely a file.
  auto obj = _metadataService.lookup(bucket, key);
  if (obj.ok()) {
    return GEDSFileStatus{.key = key, .size = obj->info.size, .isDirectory = false};
  }
  auto s3 = _objectStores.get(bucket);
  if (s3.ok()) {
    auto s3Status = (*s3)->fileStatus(bucket, key);
    if (s3Status.ok()) {
      return s3Status;
    }
    if (s3Status.status().code() != absl::StatusCode::kNotFound) {
      return s3Status.status();
    }
  }
  if (delimiter != 0) {
    // Try listing as a directory.
    return listDir(key + delimiter);
  }
  return absl::NotFoundError("Key " + key + " not found!");
}

absl::Status GEDS::renamePrefix(const std::string &bucket, const std::string &srcKey,
                                const std::string &destKey) {
  return renamePrefix(bucket, srcKey, bucket, destKey);
}

absl::Status GEDS::renamePrefix(const std::string &srcBucket, const std::string &srcKey,
                                const std::string &destBucket, const std::string &destKey) {
  LOG_DEBUG("rename", srcBucket, "/", srcKey, " to ", destBucket, "/", destKey);
  auto prefixList = list(srcBucket, srcKey);
  if (!prefixList.ok()) {
    return prefixList.status();
  }
  for (const auto &element : *prefixList) {
    if (element.isDirectory) {
      continue;
    }
    const auto &key = element.key;
    auto newKey = destKey + key.substr(srcKey.size());
    auto status = rename(srcBucket, element.key, destBucket, newKey);
    if (!status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

absl::Status GEDS::rename(const std::string &bucket, const std::string &srcKey,
                          const std::string &destKey) {
  return rename(bucket, srcKey, bucket, destKey);
}
absl::Status GEDS::rename(const std::string &srcBucket, const std::string &srcKey,
                          const std::string &destBucket, const std::string &destKey) {
  // ToDo: Actually move the files.
  auto status = copy(srcBucket, srcKey, destBucket, destKey);
  if (!status.ok()) {
    return status;
  }
  return deleteObject(srcBucket, srcKey);
}

absl::Status GEDS::copyPrefix(const std::string &bucket, const std::string &srcKey,
                              const std::string &destKey) {
  return copyPrefix(bucket, srcKey, bucket, destKey);
}

absl::Status GEDS::copyPrefix(const std::string &srcBucket, const std::string &srcKey,
                              const std::string &destBucket, const std::string &destKey) {
  auto prefixList = list(srcBucket, srcKey);
  if (!prefixList.ok()) {
    return prefixList.status();
  }
  for (const auto &element : *prefixList) {
    if (element.isDirectory) {
      continue;
    }
    const auto &key = element.key;
    auto newKey = destKey + key.substr(srcKey.size());
    auto status = copy(srcBucket, element.key, destBucket, newKey);
    if (!status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

absl::Status GEDS::copy(const std::string &bucket, const std::string &srcKey,
                        const std::string &destKey) {
  return copy(bucket, srcKey, bucket, destKey);
}

absl::Status GEDS::copy(const std::string &srcBucket, const std::string &srcKey,
                        const std::string &destBucket, const std::string &destKey) {
  auto srcFile = open(srcBucket, srcKey);
  if (!srcFile.ok()) {
    return srcFile.status();
  }
  auto destFile = create(destBucket, destKey);
  if (!destFile.ok()) {
    return destFile.status();
  }
  auto status = srcFile->copyTo(*destFile);
  if (status.ok()) {
    return destFile->seal();
  }
  return status;
}

absl::Status GEDS::deleteObject(const std::string &bucket, const std::string &key) {
  LOG_DEBUG("DeleteObject ", bucket, "/", key);
  // Delete on metadata service.
  {
    auto status = _metadataService.deleteObject(bucket, key);
    if (!status.ok() && status.code() != absl::StatusCode::kNotFound) {
      // Omit local deletion if we cannot communicate with Metadata service.
      return status;
    }
  }
  // Delete on s3.
  {
    auto storeStatus = _objectStores.get(bucket);
    if (storeStatus.ok() && !key.starts_with(GEDSCachedFileHandle::CacheBlockMarker)) {
      auto deleteStatus = storeStatus.value()->deleteObject(bucket, key);
      if (!deleteStatus.ok()) {
        LOG_ERROR("Unable to delete ", bucket, "/", key, " on S3:", deleteStatus.message());
      }
    }
  }

  // Delete the file locally.
  auto path = getPath(bucket, key);
  auto removed = _fileHandles.remove(path);
  if (!removed) {
    LOG_ERROR("The file ", path.name, " did not exist locally!");
  }
  return absl::OkStatus();
}

absl::Status GEDS::deleteObjectPrefix(const std::string &bucket, const std::string &prefix) {
  LOG_DEBUG("deleteObjectPrefix ", bucket, "/", prefix);

  // Delete on GEDS.
  {
    auto status = _metadataService.deleteObjectPrefix(bucket, prefix);
    if (!status.ok() && status.code() != absl::StatusCode::kNotFound) {
      // Omit local deletion if we cannot communicate with Metadata service.
      return status;
    }
  }
  // Delete on S3.
  {
    auto storeStatus = _objectStores.get(bucket);
    if (storeStatus.ok()) {
      auto deleteStatus = storeStatus.value()->deletePrefix(bucket, prefix);
      if (!deleteStatus.ok()) {
        LOG_ERROR("Unable to delete prefix ", bucket, "/", prefix,
                  " on S3: ", deleteStatus.message());
      }
    }
  }
  // Mark the file as deleted and remove it.
  _fileHandles.removeRange(utility::PathPrefixProbe{prefix});
  return absl::OkStatus();
}

absl::Status GEDS::registerObjectStoreConfig(const std::string &bucket,
                                             const std::string &endpointUrl,
                                             const std::string &accessKey,
                                             const std::string &secretKey) {
  auto status = _metadataService.registerObjectStoreConfig(
      ObjectStoreConfig(bucket, endpointUrl, accessKey, secretKey));
  if (!status.ok()) {
    return status;
  }
  status = createBucket(bucket);
  if (!status.ok()) {
    return status;
  }
  // Trigger update.
  return syncObjectStoreConfigs();
}

absl::Status GEDS::syncObjectStoreConfigs() {
  auto configs = _metadataService.listObjectStoreConfigs();
  if (!configs.ok()) {
    return configs.status();
  }
  for (const auto &c : configs.value()) {
    auto status =
        _objectStores.registerStore(c->bucket, c->endpointURL, c->accessKey, c->secretKey);
    if (!status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}
