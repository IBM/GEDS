/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "MDSKVS.h"
#include "ObjectStoreConfig.h"
#include "ObjectStoreHandler.h"

absl::Status PopulateKVS(std::shared_ptr<geds::ObjectStoreConfig> config, std::shared_ptr<MDSKVS> kvs);
