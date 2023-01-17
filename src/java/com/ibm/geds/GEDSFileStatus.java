/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package com.ibm.geds;

public class GEDSFileStatus {
    public final String key;
    public final long size;
    public final boolean isDirectory;

    GEDSFileStatus(String key, long size, boolean isDirectory) {
        this.key = key;
        this.size = size;
        this.isDirectory = isDirectory;
    }
}
