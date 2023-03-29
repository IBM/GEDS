/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package com.ibm.geds;

import java.io.FileNotFoundException;
import java.io.IOException;

public class GEDS {
    static {
        System.loadLibrary("geds_java");
    }

    private long nativePtr = 0;
    private final GEDSConfig config;

    public GEDS(GEDSConfig config) {
        this.config = config;
        this.nativePtr = initGEDS(this.config.getNativePtr());
    }

    /**
     * Constructor for GEDS.
     *
     * @param metadataServiceAddress Metadata server address. Format:
     *                               `hostname:port`
     * @param pathPrefix             Local data folder.
     * @param hostname               Hostname of the GEDS instance (Optional). Use
     *                               empty string if GEDS should use `hostname`.
     * @param port                   Port of the GEDS instance (Optional). `0`
     *                               indicates default port.
     * @param blockSize              Block Size used for prefetching data
     *                               (Optional). `0` indicates default block size.
     */
    @Deprecated
    public GEDS(String metadataServiceAddress, String pathPrefix, String hostname, int port, long blockSize) {
        if (port < 0 || port > 65535) {
            throw new IllegalArgumentException("Invalid port.");
        }
        if (blockSize < 0) {
            throw new IllegalArgumentException("Invalid block size");
        }
        config = new GEDSConfig(metadataServiceAddress);
        if (port != 0) {
            config.set("port", port);
        }
        if (hostname != "") {
            config.set("hostname", hostname);
        }
        if (pathPrefix != "") {
            config.set("local_storage_path", pathPrefix);
        }
        if (blockSize != 0) {
            config.set("cache_block_size", blockSize);
        }
        this.nativePtr = initGEDS(config.getNativePtr());
    }

    @Deprecated
    public GEDS(String metadataServiceAddress, String pathPrefix, int port, long blockSize) {
        this(metadataServiceAddress, pathPrefix, "", port, blockSize);
    }

    @Deprecated
    public GEDS(String metadataServiceAddress, String pathPrefix, long blockSize) {
        this(metadataServiceAddress, pathPrefix, "", 0, blockSize);
    }

    @Deprecated
    public GEDS(String metadataServiceAddress, String pathPrefix) {
        this(metadataServiceAddress, pathPrefix, "", 0, 0);
    }

    @Deprecated
    public GEDS(String metadataServiceAddress) {
        this(metadataServiceAddress, "", "", 0, 0);
    }

    private static native long initGEDS(long nativePtrConfig);

    public void stopGEDS() {
        checkGEDS();
        nativeStopGEDS(nativePtr);
    }

    private static native void nativeStopGEDS(long ptr);

    public static native void printStatistics();

    private void checkGEDS() {
        if (nativePtr == 0) {
            throw new NullPointerException("GEDS is not properly initialized!");
        }
    }

    public static native int getDefaultGEDSPort();

    public static native int getDefaultMetdataServerPort();

    /**
     * Create a file in bucket at key.
     */
    public GEDSFile create(String bucket, String key) throws IOException {
        checkGEDS();
        long ptr = nativeCreate(nativePtr, bucket, key);
        if (ptr == 0) {
            throw new RuntimeException("Unable to create file at " + bucket + "/" + key);
        }
        return new GEDSFile(ptr, bucket, key);
    }

    private native static long nativeCreate(long ptr, String bucket, String key) throws IOException;

    /**
     * Open a file in bucket at key.
     */
    public GEDSFile open(String bucket, String key) throws IOException, FileNotFoundException {
        if (nativePtr == 0) {
            throw new RuntimeException("GEDS is not initialized!");
        }
        long ptr = nativeOpen(nativePtr, bucket, key);
        if (ptr == 0) {
            throw new RuntimeException("Unable to open file " + bucket + "/" + key);
        }
        return new GEDSFile(ptr, bucket, key);
    }

    private native static long nativeOpen(long ptr, String bucket, String key)
            throws IOException, FileNotFoundException;

    /**
     * Create bucket bucket.
     */
    public boolean createBucket(String bucket) throws IOException {
        checkGEDS();
        return nativeCreateBucket(nativePtr, bucket);
    }

    private native static boolean nativeCreateBucket(long ptr, String bucket) throws IOException;

    /**
     * Recursively create folder structure.
     */
    public boolean mkdirs(String bucket, String path) throws IOException {
        checkGEDS();
        return nativeMkdirs(nativePtr, bucket, path);
    }

    private native static boolean nativeMkdirs(long ptr, String bucket, String path) throws IOException;

    /**
     * Rename an object from source to destination.
     */
    public boolean rename(String bucket, String srcKey, String destKey)
            throws IOException, FileNotFoundException {
        return rename(bucket, srcKey, bucket, destKey);
    }

    public boolean rename(String srcBucket, String srcKey, String destBucket, String destKey)
            throws IOException, FileNotFoundException {
        checkGEDS();
        return nativeRename(nativePtr, srcBucket, srcKey, destBucket, destKey);
    }

    private native static boolean nativeRename(long ptr, String srcBucket, String srcKey, String destBucket,
            String destKey) throws IOException, FileNotFoundException;

    /**
     * Rename a prefix from source to destination.
     */
    public boolean renamePrefix(String bucket, String srcKey, String destKey)
            throws IOException, FileNotFoundException {
        return renamePrefix(bucket, srcKey, bucket, destKey);
    }

    public boolean renamePrefix(String srcBucket, String srcKey, String destBucket, String destKey)
            throws IOException, FileNotFoundException {
        checkGEDS();
        return nativeRenamePrefix(nativePtr, srcBucket, srcKey, destBucket, destKey);
    }

    private native static boolean nativeRenamePrefix(long ptr, String srcBucket, String srcKey, String destBucket,
            String destKey) throws IOException, FileNotFoundException;

    /**
     * Copy an object
     */
    public boolean copy(String bucket, String srcKey, String destKey) throws IOException, FileNotFoundException {
        return copy(bucket, srcKey, bucket, destKey);
    }

    public boolean copy(String srcBucket, String srcKey, String destBucket, String destKey)
            throws IOException, FileNotFoundException {
        checkGEDS();
        return nativeCopy(nativePtr, srcBucket, srcKey, destBucket, destKey);
    }

    private native static boolean nativeCopy(long ptr, String srcBucket, String srcKey, String destBucket,
            String destKey) throws IOException, FileNotFoundException;

    /**
     * Copy objects starting with prefix.
     */
    public boolean copyPrefix(String bucket, String srcPrefix, String destPrefix)
            throws IOException, FileNotFoundException {
        return copy(bucket, srcPrefix, bucket, destPrefix);
    }

    public boolean copyPrefix(String srcBucket, String srcPrefix, String destBucket, String destPrefix)
            throws IOException, FileNotFoundException {
        checkGEDS();
        return nativeCopyPrefix(nativePtr, srcBucket, srcPrefix, destBucket, destPrefix);
    }

    private native static boolean nativeCopyPrefix(long ptr, String srcBucket, String srcPrefix, String destBucket,
            String destPrefix) throws IOException, FileNotFoundException;

    /**
     * Delete object in `bucket` with `key`.
     */
    public boolean delete(String bucket, String key) throws IOException, FileNotFoundException {
        checkGEDS();
        return nativeDelete(nativePtr, bucket, key);
    }

    private native static boolean nativeDelete(long ptr, String bucket, String key)
            throws IOException, FileNotFoundException;

    /**
     * Delete objects in `bucket` where the key starts with `prefix`.
     */
    public boolean deletePrefix(String bucket, String prefix) throws IOException {
        checkGEDS();
        return nativeDeletePrefix(nativePtr, bucket, prefix);
    }

    private native static boolean nativeDeletePrefix(long ptr, String bucket, String prefix) throws IOException;

    public GEDSFileStatus[] listAsFolder(String bucket, String key) throws IOException {
        checkGEDS();
        return nativeList(nativePtr, bucket, key, '/');
    }

    public GEDSFileStatus[] list(String bucket, String key) throws IOException {
        checkGEDS();
        return nativeList(nativePtr, bucket, key, '\0');
    }

    private native static GEDSFileStatus[] nativeList(long ptr, String bucket, String key, char delimiter)
            throws IOException;

    public GEDSFileStatus status(String bucket, String key) throws IOException, FileNotFoundException {
        checkGEDS();
        return nativeStatus(nativePtr, bucket, key, '/');
    }

    private native static GEDSFileStatus nativeStatus(long ptr, String bucket, String key, char delimiter)
            throws IOException, FileNotFoundException;

    public void registerObjectStoreConfig(String bucket, String endPointUrl, String accessKey, String secretKey) {
        checkGEDS();
        nativeRegisterObjectStoreConfig(nativePtr, bucket, endPointUrl, accessKey, secretKey);
    }

    private native void nativeRegisterObjectStoreConfig(long ptr, String bucket, String endPointUrl, String accessKey,
            String secretKey);

    public void syncObjectStoreConfigs() {
        checkGEDS();
        nativeSyncObjectStoreConfigs(nativePtr);
    }

    private native void nativeSyncObjectStoreConfigs(long ptr);
}
