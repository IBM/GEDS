/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#include <absl/status/status.h>

#include "status.pb.h"

inline auto convertStatus(geds::rpc::StatusCode code) {
  return static_cast<absl::StatusCode>(code);
}

inline auto convertStatus(absl::StatusCode code) {
  return static_cast<geds::rpc::StatusCode>(code);
}

inline auto convertStatus(const geds::rpc::StatusResponse &response) {
  auto code = convertStatus(response.code());
  if (code == absl::StatusCode::kOk) {
    return absl::OkStatus();
  }
  return absl::Status(code, response.message());
}

inline void convertStatus(geds::rpc::StatusResponse *response, const absl::Status &status) {
  response->set_code(convertStatus(status.code()));
  if (!status.ok()) {
    response->set_message(std::string{status.message()});
  }
}
