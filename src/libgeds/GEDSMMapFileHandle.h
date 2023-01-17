/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "GEDSAbstractFileHandle.h"
#include "MMAPFile.h"

using GEDSMMapFileHandle = GEDSAbstractFileHandle<geds::filesystem::MMAPFile>;
