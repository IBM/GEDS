/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
#pragma once

#include "GEDSAbstractFileHandle.h"
#include "LocalFile.h"

using GEDSLocalFileHandle = GEDSAbstractFileHandle<geds::filesystem::LocalFile>;
