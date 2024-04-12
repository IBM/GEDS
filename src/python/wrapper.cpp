/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
      .def_readwrite("cache_block_size", &GEDSConfig::cacheBlockSize)
      .def_readwrite("cache_objects_from_s3", &GEDSConfig::cache_objects_from_s3)
      .def_readwrite("available_local_storage", &GEDSConfig::available_local_storage)
      .def_readwrite("available_local_memory", &GEDSConfig::available_local_memory);

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
          py::arg("bucket"), py::arg("key"), py::arg("overwrite") = true,
          py::call_guard<py::gil_scoped_release>())
      .def("create_bucket", &GEDS::createBucket, py::call_guard<py::gil_scoped_release>())
      .def(
          "mkdirs",
          [](GEDS &self, const std::string &bucket, const std::string &path) -> absl::Status {
            return self.mkdirs(bucket, path);
          },
          py::call_guard<py::gil_scoped_release>())
      .def(
          "list",
          [](GEDS &self, const std::string &bucket, const std::string &key)
              -> absl::StatusOr<std::vector<GEDSFileStatus>> { return self.list(bucket, key); },
          py::call_guard<py::gil_scoped_release>())
      .def("list_folder", &GEDS::listAsFolder, py::call_guard<py::gil_scoped_release>())
      .def(
          "status",
          [](GEDS &self, const std::string &bucket, const std::string &key)
              -> absl::StatusOr<GEDSFileStatus> { return self.status(bucket, key); },
          py::call_guard<py::gil_scoped_release>())
      .def(
          "open",
          [](GEDS &self, const std::string &bucket, const std::string &key,
             bool retry) -> absl::StatusOr<GEDSFile> { return self.open(bucket, key, retry); },
          py::arg("bucket"), py::arg("key"), py::arg("retry") = false)
      .def("delete", &GEDS::deleteObject, py::call_guard<py::gil_scoped_release>())
      .def("delete_prefix", &GEDS::deleteObjectPrefix, py::call_guard<py::gil_scoped_release>())
      .def(
          "rename",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcKey,
             const std::string &destBucket, const std::string &destKey) -> absl::Status {
            return self.rename(srcBucket, srcKey, destBucket, destKey);
          },
          py::arg("src_bucket"), py::arg("src_key"), py::arg("dest_bucket"), py::arg("dest_key"),
          py::call_guard<py::gil_scoped_release>())
      .def(
          "rename_prefix",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcPrefix,
             const std::string &destBucket, const std::string &destPrefix) -> absl::Status {
            return self.renamePrefix(srcBucket, srcPrefix, destBucket, destPrefix);
          },
          py::arg("src_bucket"), py::arg("src_prefix"), py::arg("dest_bucket"),
          py::arg("dest_prefix"), py::call_guard<py::gil_scoped_release>())
      .def(
          "copy",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcKey,
             const std::string &destBucket, const std::string &destKey) -> absl::Status {
            return self.copy(srcBucket, srcKey, destBucket, destKey);
          },
          py::arg("src_bucket"), py::arg("src_key"), py::arg("dest_bucket"), py::arg("dest_key"),
          py::call_guard<py::gil_scoped_release>())
      .def(
          "copy_prefix",
          [](GEDS &self, const std::string &srcBucket, const std::string &srcPrefix,
             const std::string &destBucket, const std::string &destPrefix) -> absl::Status {
            return self.copyPrefix(srcBucket, srcPrefix, destBucket, destPrefix);
          },
          py::arg("src_bucket"), py::arg("src_prefix"), py::arg("dest_bucket"),
          py::arg("dest_prefix"), py::call_guard<py::gil_scoped_release>())
      .def(
          "local_path",
          [](GEDS &self, GEDSFile &file) -> std::string { return self.getLocalPath(file); },
          py::call_guard<py::gil_scoped_release>())
      .def("registerObjectStoreConfig", &GEDS::registerObjectStoreConfig, py::arg("bucket"),
           py::arg("endpointUrl"), py::arg("accessKey"), py::arg("secretKey"),
           py::call_guard<py::gil_scoped_release>())
      .def("syncObjectStoreConfigs", &GEDS::syncObjectStoreConfigs,
           py::call_guard<py::gil_scoped_release>())
      .def(
          "relocate", [](GEDS &self, bool force) { self.relocate(force); },
          py::arg("force") = false, py::call_guard<py::gil_scoped_release>());

  py::class_<GEDSFile, std::shared_ptr<GEDSFile>>(m, "GEDSFile")
      .def_property_readonly("size", &GEDSFile::size)
      .def_property_readonly("writeable", &GEDSFile::isWriteable)
      .def_property_readonly("identifier", &GEDSFile::identifier)
      .def_property_readonly("metadata", &GEDSFile::metadata,
                             py::call_guard<py::gil_scoped_release>())
      .def_property_readonly("metadata_bytes",
                             [](GEDSFile &file) -> std::optional<py::bytes> {
                               std::optional<std::string> s;
                               {
                                 py::gil_scoped_release release;
                                 s = file.metadata();
                               }
                               if (s->empty()) {
                                 return std::nullopt;
                               }
                               return std::make_optional(py::bytes(s.value()));
                             })
      .def("truncate", &GEDSFile::truncate, py::call_guard<py::gil_scoped_release>())
      .def("raw_ptr", &GEDSFile::rawPtr, py::call_guard<py::gil_scoped_release>())
      .def("seal", &GEDSFile::seal, py::call_guard<py::gil_scoped_release>())
      .def(
          "set_metadata",
          [](GEDSFile &self, std::optional<std::string> metadata, bool seal) -> absl::Status {
            py::gil_scoped_release release;
            return self.setMetadata(metadata, seal);
          },
          py::arg("metadata"), py::arg("seal") = true)
      .def(
          "set_metadata",
          [](GEDSFile &self, const py::buffer buffer, std::optional<size_t> lengthArg,
             bool seal) -> absl::Status {
            py::buffer_info info = buffer.request();
            if (info.ndim != 1) {
              return absl::FailedPreconditionError("Buffer has wrong dimensions!");
            }
            size_t length = info.size;
            if (lengthArg.has_value()) {
              length = std::min(length, lengthArg.value());
            }
            py::gil_scoped_release release;
            return self.setMetadata(static_cast<const uint8_t *>(info.ptr), length, seal);
          },
          py::arg("buffer"), py::arg("length") = std::nullopt, py::arg("seal") = true)
      .def(
          "set_metadata",
          [](GEDSFile &self, const char *buffer, std::optional<size_t> lengthArg,
             bool seal) -> absl::Status {
            size_t length = lengthArg.value_or(strlen(buffer));
            py::gil_scoped_release release;
            return self.setMetadata(reinterpret_cast<const uint8_t *>(buffer), length, seal);
          },
          py::arg("buffer"), py::arg("length") = std::nullopt, py::arg("seal") = true)
      .def("read",
           [](GEDSFile &self, py::buffer buffer, size_t position,
              size_t length) -> absl::StatusOr<size_t> {
             py::buffer_info info = buffer.request(true);
             if (info.ndim != 1) {
               return absl::FailedPreconditionError("Buffer has wrong dimensions!");
             }
             if ((size_t)info.size < length) {
               return absl::FailedPreconditionError("The buffer does not have sufficient space!");
             }
             length = std::min<size_t>(info.size, length);
             py::gil_scoped_release release;
             return self.read(static_cast<uint8_t *>(info.ptr), position, length);
           })
      .def("read",
           [](GEDSFile &self, char *array, size_t position,
              size_t length) -> absl::StatusOr<size_t> {
             py::gil_scoped_release release;
             return self.read(reinterpret_cast<uint8_t *>(array), position, length);
           })
      .def("write",
           [](GEDSFile &self, const py::buffer buffer, size_t position,
              size_t length) -> absl::Status {
             py::buffer_info info = buffer.request();
             if (info.ndim != 1) {
               return absl::FailedPreconditionError("Buffer has wrong dimensions!");
             }
             if ((size_t)info.size < length) {
               return absl::FailedPreconditionError("The buffer does not have sufficient space!");
             }
             length = std::min<size_t>(info.size, length);
             py::gil_scoped_release release;
             return self.write(static_cast<const uint8_t *>(info.ptr), position, length);
           })
      .def("write",
           [](GEDSFile &self, const char *array, size_t position, size_t length) -> absl::Status {
             py::gil_scoped_release release;
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
