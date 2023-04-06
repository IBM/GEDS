/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

package com.ibm.geds;

import java.io.IOException;
import java.nio.ByteBuffer;

import sun.nio.ch.DirectBuffer;

public class GEDSFile {
    private long nativePtr = 0;
    public final String bucket;
    public final String key;

    GEDSFile(long nativePtr, String bucket, String key) {
        this.nativePtr = nativePtr;
        this.bucket = bucket;
        this.key = key;
    }

    public synchronized void close() throws IOException {
        if (!isClosed()) {
            closeNative(nativePtr);
            nativePtr = 0;
        }
    }

    public synchronized boolean isClosed() {
        return nativePtr == 0;
    }

    private synchronized void checkClosed() throws IOException {
        if (nativePtr == 0) {
            throw new IOException("The file is already closed!");
        }
    }

    public long size() throws IOException {
        checkClosed();
        return sizeNative(nativePtr);
    }

    private native long sizeNative(long nativePtr);

    private void checkPosition(long position) {
        if (position < 0) {
            throw new IllegalArgumentException("Invalid position!");
        }
    }

    private void checkBuffer(byte[] buffer, int offset, int length) {
        if (length < 0) {
            throw new IllegalArgumentException("Invalid length!");
        }
        if (offset < 0) {
            throw new IllegalArgumentException("Invalid offset!");
        }
        if ((long) buffer.length < (long) offset + (long) length) {
            throw new IllegalArgumentException("The buffer is too small!");
        }
    }

    public int read(long position, byte[] buffer, int offset, int length) throws IOException {
        checkClosed();
        checkPosition(position);
        checkBuffer(buffer, offset, length);
        return readNative(nativePtr, position, buffer, offset, length);
    }

    /**
     * Reads up to buffer.remaining() bytes at buffer.position() into buffer.
     * buffer.remaining() can be
     * controlled by configuring buffer.limit().
     */
    public int read(long position, ByteBuffer buffer) throws IOException {
        checkClosed();

        int offset = buffer.position();
        int length = buffer.remaining();
        if (length == 0) {
            return 0;
        }
        int numBytes;
        if (buffer.isDirect()) {
            long addr = ((DirectBuffer) buffer).address();
            numBytes = readNative(nativePtr, position, addr, offset, length);
        } else {
            numBytes = read(position, buffer.array(), offset, length);
        }
        if (numBytes > 0) {
            buffer.position(offset + numBytes);
        }
        return numBytes;
    }

    public void write(long position, byte[] buffer, int offset, int length) throws IOException {
        checkClosed();
        checkPosition(position);
        checkBuffer(buffer, offset, length);
        writeNative(nativePtr, position, buffer, offset, length);
    }

    /**
     * Writes buffer at position.
     */
    public int write(long position, ByteBuffer buffer) throws IOException {
        checkClosed();
        int offset = buffer.position();
        int length = buffer.remaining();
        if (length == 0) {
            return 0;
        }
        if (buffer.isDirect()) {
            long addr = ((DirectBuffer) buffer).address();
            writeNative(nativePtr, position, addr, offset, length);
        } else {
            write(position, buffer.array(), offset, length);
        }
        buffer.position(offset + length);
        return length;
    }

    /**
     * Seal the file.
     */
    public void seal() throws IOException {
        checkClosed();
        sealNative(nativePtr);
    }

    /**
     * Truncate a file to a target size.
     */
    public void truncate(long targetSize) throws IOException {
        checkClosed();
        if (targetSize < 0) {
            throw new IllegalArgumentException("Invalid target size!");
        }
        truncateNative(nativePtr, targetSize);
    }

    public boolean isWriteable() throws IOException {
        checkClosed();
        return isWritableNative(nativePtr);
    }

    private native void closeNative(long nativePtr) throws IOException;

    private native int readNative(long nativePtr, long position, byte[] buffer, int offset, int length) throws IOException;

    private native void writeNative(long nativePtr, long position, byte[] buffer, int offset, int length) throws IOException;

    private native int readNative(long nativePtr, long position, long ptr, int offset, int length) throws IOException;

    private native void writeNative(long nativePtr, long position, long ptr, int offset, int length) throws IOException;

    private native void sealNative(long nativePtr) throws IOException;

    private native void truncateNative(long nativePtr, long targetSize) throws IOException;

    private native boolean isWritableNative(long nativePtr) throws IOException;
}
