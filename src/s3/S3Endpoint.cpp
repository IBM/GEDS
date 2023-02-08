/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "S3Endpoint.h"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <istream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <absl/status/status.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/Scheme.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/S3Errors.h>
#include <aws/s3/model/Delete.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectResult.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/ObjectIdentifier.h>
#include <aws/s3/model/PutObjectRequest.h>

#include "ByteStream.h"
#include "DirectoryMarker.h"
#include "GEDSFileStatus.h"
#include "Logging.h"
#include "S3Init.h"

static bool urlIsHttps(const std::string &url) {
  const std::string_view HTTP = "http://";
  if (url.size() >= HTTP.size() && url.compare(0, HTTP.size(), HTTP) == 0) {
    return false;
  }
  // Assume true otherwise.
  return true;
}
static std::string stripHttp(const std::string &url) {
  {
    const std::string_view HTTP = "http://";
    if (url.size() >= HTTP.size() && url.compare(0, HTTP.size(), HTTP) == 0) {
      return url.substr(HTTP.size());
    }
  }
  {
    const std::string_view HTTPS = "http://";
    if (url.size() >= HTTPS.size() && url.compare(0, HTTPS.size(), HTTPS) == 0) {
      return url.substr(HTTPS.size());
    }
  }
  return url;
}

namespace geds::s3 {
Endpoint::Endpoint(std::string argEndpointUrl, std::string argAccessKey, std::string secretKey)
    : _endpointUrl(std::move(argEndpointUrl)), _accessKey(std::move(argAccessKey)),
      _s3Credentials(_accessKey, secretKey) {
  Init();
  _s3Config = std::make_shared<Aws::Client::ClientConfiguration>(); // Requires Init();
  _s3Config->endpointOverride = stripHttp(_endpointUrl);
  _s3Config->scheme = urlIsHttps(_endpointUrl) ? Aws::Http::Scheme::HTTPS : Aws::Http::Scheme::HTTP;
  _s3Client = std::make_shared<Aws::S3::S3Client>(
      _s3Credentials, *_s3Config, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);
}

absl::Status Endpoint::convertS3Error(const Aws::S3::S3Error &error, const std::string &action,
                                      const std::string &key) const {
  const auto &errorType = error.GetErrorType();
  const auto &errorCode = error.GetResponseCode();
  if (errorType == Aws::S3::S3Errors::NO_SUCH_KEY ||
      errorCode == Aws::Http::HttpResponseCode::NOT_FOUND) {
    return absl::NotFoundError("Unable to " + action + ": the " + key + " does not exist!");
  }
  if (errorCode == Aws::Http::HttpResponseCode::UNAUTHORIZED ||
      errorType == Aws::S3::S3Errors::ACCESS_DENIED) {
    return absl::PermissionDeniedError(error.GetMessage());
  }
  return absl::UnknownError("Unable to " + action + ": " + key + ": " + error.GetMessage());
}

Endpoint::~Endpoint() {}

absl::StatusOr<std::vector<GEDSFileStatus>> Endpoint::list(const std::string &bucket,
                                                           const std::string &prefix) const {
  return list(bucket, prefix, 0);
}

absl::StatusOr<std::vector<GEDSFileStatus>>
Endpoint::list(const std::string &bucket, const std::string &prefix, char delimiter,
               std::optional<std::string> continuationToken) const {
  std::set<GEDSFileStatus> result;
  auto status = list(bucket, prefix, delimiter, result, continuationToken);
  if (!status.ok()) {
    return status;
  }
  return std::vector<GEDSFileStatus>{result.begin(), result.end()};
}

absl::Status Endpoint::list(const std::string &bucket, const std::string &prefix, char delimiter,
                            std::set<GEDSFileStatus> &result,
                            std::optional<std::string> continuationToken) const {
  LOG_DEBUG(bucket, "/", prefix, "with ", std::to_string(delimiter));
  Aws::S3::Model::ListObjectsV2Request request;
  request.WithBucket(bucket);
  request.WithPrefix(prefix);
  if (delimiter != 0) {
    request.WithDelimiter(std::string{delimiter});
  }
  if (continuationToken.has_value()) {
    request.WithContinuationToken(*continuationToken);
  }

  *totalRequestsSent += 1;
  auto outcome = _s3Client->ListObjectsV2(request);
  if (!outcome.IsSuccess()) {
    auto &error = outcome.GetError();
    return convertS3Error(error, "list", prefix);
  }
  const auto &s3result = outcome.GetResult();
  const auto &objects = s3result.GetContents();
  const auto &prefixes = s3result.GetCommonPrefixes();

  auto newToken = s3result.GetNextContinuationToken();
  if (newToken != "") {
    // LOG_DEBUG("Sending sub request");
    auto subRequest = list(bucket, prefix, delimiter, result, newToken);
    if (!subRequest.ok()) {
      return subRequest;
    }
  }
  // result.reserve(result.size() + objects.size() + prefixes.size());
  const std::string folderString = delimiter + Default_DirectoryMarker;
  for (auto &obj : objects) {
    const auto &key = obj.GetKey();
    if (delimiter != 0 && key.ends_with(folderString)) {
      // Don't list current directory.
      continue;
    }
    auto size = (size_t)obj.GetSize();
    result.emplace(GEDSFileStatus{.key = key, .size = size, .isDirectory = false});
  }
  for (auto &prefix : prefixes) {
    const auto &key = prefix.GetPrefix();
    result.emplace(GEDSFileStatus{.key = key, .size = 0, .isDirectory = true});
  }
  return absl::OkStatus();
}

absl::StatusOr<std::vector<GEDSFileStatus>>
Endpoint::listAsFolder(const std::string &bucket, const std::string &prefix) const {
  return list(bucket, prefix, '/');
}

absl::StatusOr<GEDSFileStatus> Endpoint::fileStatus(const std::string &bucket,
                                                    const std::string &key) const {
  Aws::S3::Model::HeadObjectRequest request;
  request.SetBucket(bucket);
  request.SetKey(key);

  *totalRequestsSent += 1;
  auto outcome = _s3Client->HeadObject(request);
  if (!outcome.IsSuccess()) {
    auto &error = outcome.GetError();
    return convertS3Error(error, "file status", key);
  }
  auto &result = outcome.GetResult();
  return GEDSFileStatus{
      .key = key, .size = (size_t)(result.GetContentLength()), .isDirectory = false};
}

absl::StatusOr<GEDSFileStatus>
Endpoint::folderStatus(const std::string &bucket, const std::string &key, char delimiter) const {
  Aws::S3::Model::ListObjectsV2Request request;

  request.WithBucket(bucket);
  request.WithPrefix(key);
  request.WithMaxKeys(1);

  *totalRequestsSent += 1;
  auto outcome = _s3Client->ListObjectsV2(request);
  if (!outcome.IsSuccess()) {
    auto &error = outcome.GetError();
    return convertS3Error(error, "folder status", key);
  }
  const auto &result = outcome.GetResult();
  if (!result.GetContents().empty() || !result.GetCommonPrefixes().empty() ||
      (key.size() == 1 && key[0] == delimiter)) {
    return GEDSFileStatus{.key = key, .size = 0, .isDirectory = true};
  }
  return absl::NotFoundError("Folder " + key + " not found.");
}

absl::Status Endpoint::deleteObject(const std::string &bucket, const std::string &key) {
  Aws::S3::Model::DeleteObjectRequest request;
  request.WithBucket(bucket);
  request.WithKey(key);

  *totalRequestsSent += 1;
  auto outcome = _s3Client->DeleteObject(request);
  if (!outcome.IsSuccess()) {
    auto &error = outcome.GetError();
    return convertS3Error(error, "delete object", key);
  }
  return absl::OkStatus();
}

absl::Status Endpoint::deletePrefix(const std::string &bucket, const std::string &prefix) {

  auto fileStatus = list(bucket, prefix);
  if (!fileStatus.ok()) {
    return fileStatus.status();
  }

  auto files = fileStatus.value();

  size_t startPos = 0;
  size_t endPos = 0;

  while (startPos < files.size()) {
    endPos = std::min<size_t>(startPos + 1000, files.size());

    Aws::S3::Model::Delete objects;
    for (size_t i = startPos; i < endPos; i++) {
      Aws::S3::Model::ObjectIdentifier id;
      id.SetKey(files[i].key);
      objects.AddObjects(id);
    }

    Aws::S3::Model::DeleteObjectsRequest request;
    request.WithBucket(bucket);
    request.WithDelete(std::move(objects));

    *totalRequestsSent += 1;
    auto outcome = _s3Client->DeleteObjects(request);
    if (!outcome.IsSuccess()) {
      auto &error = outcome.GetError();
      return convertS3Error(error, "delete prefix", prefix);
    }

    startPos = endPos;
  }
  return absl::OkStatus();
}

absl::Status Endpoint::putObject(const std::string &bucket, const std::string &key,
                                 const uint8_t *bytes, size_t length) {
  // AWS SDK requests a iostream. Thus we have to remove the const from the backing store.
  auto stream = std::make_shared<utility::ByteIOStream>(const_cast<uint8_t *>(bytes), length,
                                                        std::ios_base::in);
  // stream->exceptions(std::iostream::failbit);
  return putObject(bucket, key, stream);
}

absl::Status Endpoint::putObject(const std::string &bucket, const std::string &key,
                                 std::shared_ptr<std::iostream> stream) {
  Aws::S3::Model::PutObjectRequest request;
  request.SetBucket(bucket);
  request.SetKey(key);
  request.SetBody(stream);
  request.SetContentType("application/octet-stream");

  *totalRequestsSent += 1;
  auto outcome = _s3Client->PutObject(request);
  if (!outcome.IsSuccess()) {
    auto &error = outcome.GetError();
    return convertS3Error(error, "put", key);
  }
  return absl::OkStatus();
}

absl::StatusOr<size_t> Endpoint::readBytes(const std::string &bucket, const std::string &key,
                                           uint8_t *bytes, size_t position, size_t length) const {

  auto stream = utility::ByteIOStream(bytes, length);
  return read(bucket, key, stream, position, length);
}

absl::StatusOr<size_t> Endpoint::read(const std::string &bucket, const std::string &key,
                                      std::iostream &outputStream, std::optional<size_t> position,
                                      std::optional<size_t> length, bool retry) const {
  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(bucket);
  request.SetKey(key);
  request.SetResponseContentType("application/octet-stream");
  if (position.has_value() || length.has_value()) {
    request.SetRange(
        "bytes=" + (position.has_value() ? std::to_string(position.value()) : "") + "-" +
        (length.has_value() ? std::to_string(position.value_or(0) + length.value()) : ""));
  }

  *totalRequestsSent += 1;
  auto outcome = _s3Client->GetObject(request);
  if (!outcome.IsSuccess()) {
    auto &error = outcome.GetError();
    if (error.GetExceptionName() == "InvalidRange" && !retry) {
      LOG_DEBUG("Endpoint reports ", bucket, "/", key, ": ", error.GetMessage());
      auto fileInfo = fileStatus(bucket, key);
      if (!fileInfo.ok()) {
        return fileInfo.status();
      }
      if (position.has_value() && position.value() >= fileInfo->size) {
        return 0;
      }
      if (length.has_value()) {
        auto newCount =
            std::min(position.value_or(0) + length.value(), fileInfo->size) - position.value_or(0);
        if (newCount == 0) {
          return 0;
        }
        LOG_DEBUG("Retrying read for ", bucket, "/", key, " with truncated count.");
        return read(bucket, key, outputStream, position, newCount, true);
      }
      return absl::InternalError("Unable to read " + bucket + "/" + key + ":" + error.GetMessage());
    }
    LOG_DEBUG("Unable to read ", bucket, "/", key, ": ", error.GetMessage());
    return convertS3Error(error, "readBytes", key);
  }

  auto &result = outcome.GetResult();
  auto &body = result.GetBody();

  auto count = (size_t)result.GetContentLength();
  if (length.has_value()) {
    // HTTP Response is null-terminated.
    count = std::min(*length, count);
  }
  *numBytesReadCount += count;
  std::copy_n(std::istreambuf_iterator<char>(body), count, std::ostreambuf_iterator(outputStream));
  return count;
}

} // namespace geds::s3
