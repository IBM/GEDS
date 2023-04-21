/**
 * Copyright 2023- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package com.ibm.geds;

public class GEDSConfig {
    static {
        System.loadLibrary("geds_java");
    }

    private long nativePtr = 0;

    public long getNativePtr() {
        return nativePtr;
    }

    public final String serverAddress;

    public GEDSConfig(String serverAddress) {
        this.serverAddress = serverAddress;
        this.nativePtr = initGEDSConfig(serverAddress);
    }

    public void set(String key, String value) {
        nativeSetString(nativePtr, key, value);
    }

    public void set(String key, int value) {
        nativeSetInt(nativePtr, key, value);
    }

    public void set(String key, long value) {
        nativeSetLong(nativePtr, key, value);
    }

    public String getString(String key) {
        return nativeGetString(nativePtr, key);
    }

    public int getInt(String key) {
        return nativeGetInt(nativePtr, key);
    }

    public long getLong(String key) {
        return nativeGetLong(nativePtr, key);
    }

    private static native long initGEDSConfig(String serverAddress);

    private static native void nativeSetString(long nativePtr, String key, String value);

    private static native void nativeSetInt(long nativePtr, String key, int value);

    private static native void nativeSetLong(long nativePtr, String key, long value);

    private static native String nativeGetString(long nativePtr, String key);

    private static native int nativeGetInt(long nativePtr, String key);

    private static native long nativeGetLong(long nativePtr, String key);
}
