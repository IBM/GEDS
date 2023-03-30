/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <pybind11/buffer_info.h>
#include <pybind11/detail/common.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11_abseil/status_casters.h>

#include "GEDS.h"
#include "GEDSConfig.h"
#include "GEDSFile.h"
#include "GEDSFileStatus.h"
#include "Platform.h"
#include "Ports.h"

namespace py = pybind11;

PYBIND11_MODULE(pygeds, m) {
  m.doc() = "GEDS plugin"; // optional module docstring
  auto statusModule = pybind11::google::ImportStatusModule();

  py::class_<GEDSConfig>(m, "GEDSConfig")
      .def(py::init([](std::string metadata_service_address) {
        return std::make_unique<GEDSConfig>(metadata_service_address);
      }))
      .def_readwrite("metadata_service_address", &GEDSConfig::metadataServiceAddress)
      .def_readwrite("listen_address", &GEDSConfig::listenAddress)
      .def_readwrite("hostname", &GEDSConfig::hostname)
      .def_readwrite("port", &GEDSConfig::port)
      .def_readwrite("port_http_server", &GEDSConfig::portHttpServer)
      .def_readwrite("local_storage_path", &GEDSConfig::localStoragePath)
      .def_readwrite("cache_block_size", &GEDSConfig::cacheBlockSize);

  py::class_<GEDS, std::shared_ptr<GEDS>>(m, "GEDS")
      .def_property_readonly_static(
          "default_port", [](py::object /* self */) -> uint16_t { return defaultGEDSPort; })
      .def_property_readonly_static(
          "default_metadata_server_port",
          [](py::object /* self */) -> uint16_t { return defaultMetdataServerPort; })
      .def(py::init<GEDSConfig>())
      .def("start", &GEDS::start)
      .def("stop", &GEDS::stop)
      .def(
          "create",
          [](GEDS &self, const std::string &bucket, const std::string &key, bool overwrite)
              -> absl::StatusOr<GEDSFile> { return self.create(bucket, key, overwrite); },
          py::arg("bucket"), py::arg("key"), py::arg("overwrite") = true)
      .def("create_bucket", &GEDS::createBucket)
      .def("mkdirs",
           [](GEDS &self, const std::string &bucket, const std::string &path) -> absl::Status {
             return self.mkdirs(bucket, path);
           })
      .def("list",
           [](GEDS &self, const std::string &bucket, const std::string &key)
               -> absl::StatusOr<std::vector<GEDSFileStatus>> { return self.list(bucket, key); })
      .def("list_folder", &GEDS::listAsFolder)
      .def("status",
           [](GEDS &self, const std::string &bucket, const std::string &key)
               -> absl::StatusOr<GEDSFileStatus> { return self.status(bucket, key); })
      .def("open", &GEDS::open)
      .def("delete", &GEDS::deleteObject)
      .def("delete_prefix", &GEDS::deleteObjectPrefix)
      .def(
          "rename",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcKey,
             const std::string &destBucket, const std::string &destKey) -> absl::Status {
            return self.rename(srcBucket, srcKey, destBucket, destKey);
          },
          py::arg("src_bucket"), py::arg("src_key"), py::arg("dest_bucket"), py::arg("dest_key"))
      .def(
          "rename_prefix",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcPrefix,
             const std::string &destBucket, const std::string &destPrefix) -> absl::Status {
            return self.renamePrefix(srcBucket, srcPrefix, destBucket, destPrefix);
          },
          py::arg("src_bucket"), py::arg("src_prefix"), py::arg("dest_bucket"),
          py::arg("dest_prefix"))
      .def(
          "copy",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcKey,
             const std::string &destBucket, const std::string &destKey) -> absl::Status {
            return self.copy(srcBucket, srcKey, destBucket, destKey);
          },
          py::arg("src_bucket"), py::arg("src_key"), py::arg("dest_bucket"), py::arg("dest_key"))
      .def(
          "copy_prefix",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcPrefix,
             const std::string &destBucket, const std::string &destPrefix) -> absl::Status {
            return self.copyPrefix(srcBucket, srcPrefix, destBucket, destPrefix);
          },
          py::arg("src_bucket"), py::arg("src_prefix"), py::arg("dest_bucket"),
          py::arg("dest_prefix"))
      .def("local_path",
           [](GEDS &self, GEDSFile &file) -> std::string { return self.getLocalPath(file); })
      .def("registerObjectStoreConfig", &GEDS::registerObjectStoreConfig, py::arg("bucket"),
           py::arg("endpointUrl"), py::arg("accessKey"), py::arg("secretKey"))
      .def("syncObjectStoreConfigs", &GEDS::syncObjectStoreConfigs);

  py::class_<GEDSFile, std::shared_ptr<GEDSFile>>(m, "GEDSFile")
      .def_property_readonly("size", &GEDSFile::size)
      .def_property_readonly("writeable", &GEDSFile::isWriteable)
      .def_property_readonly("identifier", &GEDSFile::identifier)
      .def("truncate", &GEDSFile::truncate)
      .def("seal", &GEDSFile::seal)
      .def("read",
           [](GEDSFile &self, py::array_t<uint8_t> &buffer, size_t position,
              size_t length) -> absl::StatusOr<size_t> {
             py::buffer_info info = buffer.request(true);
             if (info.ndim != 1) {
               return absl::FailedPreconditionError("Buffer has wrong dimensions!");
             }
             if ((size_t)info.size < length) {
               return absl::FailedPreconditionError("The buffer does not have sufficient space!");
             }
             length = std::min<size_t>(info.size, length);
             return self.read(static_cast<uint8_t *>(info.ptr), position, length);
           })
      .def("read",
           [](GEDSFile &self, char *array, size_t position,
              size_t length) -> absl::StatusOr<size_t> {
             return self.read(reinterpret_cast<uint8_t *>(array), position, length);
           })
      .def("write",
           [](GEDSFile &self, const py::array_t<uint8_t> &buffer, size_t position,
              size_t length) -> absl::Status {
             py::buffer_info info = buffer.request(false);
             if (info.ndim != 1) {
               return absl::FailedPreconditionError("Buffer has wrong dimensions!");
             }
             if ((size_t)info.size < length) {
               return absl::FailedPreconditionError("The buffer does not have sufficient space!");
             }
             length = std::min<size_t>(info.size, length);
             return self.write(static_cast<const uint8_t *>(info.ptr), position, length);
           })
      .def("write",
           [](GEDSFile &self, const char *array, size_t position, size_t length) -> absl::Status {
             return self.write(reinterpret_cast<const uint8_t *>(array), position, length);
           });

  py::class_<GEDSFileStatus>(m, "GEDSFileStatus")
      .def_property_readonly("key", [](GEDSFileStatus &self) -> std::string { return self.key; })
      .def_property_readonly("name", [](GEDSFileStatus &self) -> std::string { return self.key; })
      .def_property_readonly("size", [](GEDSFileStatus &self) -> size_t { return self.size; })
      .def_property_readonly("isDirectory",
                             [](GEDSFileStatus &self) -> bool { return self.isDirectory; })
      .def("__str__", [](GEDSFileStatus &self) -> std::string { return self.key; })
      .def("__repr__", [](GEDSFileStatus &self) -> std::string {
        return "{" + self.key + ", " + std::to_string(self.size) + " bytes, " +
               (self.isDirectory ? "directory" : "file") + "}";
      });
}
