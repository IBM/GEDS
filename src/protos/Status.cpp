/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Status.h"

static_assert(static_cast<int>(absl::StatusCode::kOk) ==
              static_cast<int>(geds::rpc::StatusCode::OK));
static_assert(static_cast<int>(absl::StatusCode::kCancelled) ==
              static_cast<int>(geds::rpc::StatusCode::CANCELLED));
static_assert(static_cast<int>(absl::StatusCode::kUnknown) ==
              static_cast<int>(geds::rpc::StatusCode::UNKNOWN));
static_assert(static_cast<int>(absl::StatusCode::kInvalidArgument) ==
              static_cast<int>(geds::rpc::StatusCode::INVALID_ARGUMENT));
static_assert(static_cast<int>(absl::StatusCode::kDeadlineExceeded) ==
              static_cast<int>(geds::rpc::StatusCode::DEADLINE_EXCEEDED));
static_assert(static_cast<int>(absl::StatusCode::kNotFound) ==
              static_cast<int>(geds::rpc::StatusCode::NOT_FOUND));
static_assert(static_cast<int>(absl::StatusCode::kAlreadyExists) ==
              static_cast<int>(geds::rpc::StatusCode::ALREADY_EXISTS));
static_assert(static_cast<int>(absl::StatusCode::kPermissionDenied) ==
              static_cast<int>(geds::rpc::StatusCode::PERMISSION_DENIED));
static_assert(static_cast<int>(absl::StatusCode::kResourceExhausted) ==
              static_cast<int>(geds::rpc::StatusCode::RESOURCE_EXHAUSTED));
static_assert(static_cast<int>(absl::StatusCode::kFailedPrecondition) ==
              static_cast<int>(geds::rpc::StatusCode::FAILED_PRECONDITION));
static_assert(static_cast<int>(absl::StatusCode::kAborted) ==
              static_cast<int>(geds::rpc::StatusCode::ABORTED));
static_assert(static_cast<int>(absl::StatusCode::kOutOfRange) ==
              static_cast<int>(geds::rpc::StatusCode::OUT_OF_RANGE));
static_assert(static_cast<int>(absl::StatusCode::kUnimplemented) ==
              static_cast<int>(geds::rpc::StatusCode::UNIMPLEMENTED));
static_assert(static_cast<int>(absl::StatusCode::kInternal) ==
              static_cast<int>(geds::rpc::StatusCode::INTERNAL));
static_assert(static_cast<int>(absl::StatusCode::kUnavailable) ==
              static_cast<int>(geds::rpc::StatusCode::UNAVAILABLE));
static_assert(static_cast<int>(absl::StatusCode::kDataLoss) ==
              static_cast<int>(geds::rpc::StatusCode::DATA_LOSS));
static_assert(static_cast<int>(absl::StatusCode::kUnauthenticated) ==
              static_cast<int>(geds::rpc::StatusCode::UNAUTHENTICATED));
