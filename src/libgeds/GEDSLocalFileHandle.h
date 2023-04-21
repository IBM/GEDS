/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "GEDSAbstractFileHandle.h"
#include "LocalFile.h"
#include "MMAPFile.h"

using GEDSLocalFileHandle = GEDSAbstractFileHandle<geds::filesystem::LocalFile>;
// using GEDSLocalFileHandle = GEDSAbstractFileHandle<geds::filesystem::MMAPFile>;
