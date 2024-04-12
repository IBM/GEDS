#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

# Implements smart_open for GEDS
# https://github.com/RaRe-Technologies/smart_open/blob/develop/extending.md

import io
import os
import threading
import atexit
from typing import Iterable

try:
    from . import pygeds
except ImportError:
    MISSING_DEPS = True

import smart_open.bytebuffer
import smart_open.concurrency
import smart_open.utils

from smart_open import constants

__all__ = ["SCHEME", "open", "open_uri", "parse_uri"]

SCHEME = "geds"


class GEDSRawInputBase(io.RawIOBase):
    """
    # Implements RawIOBase
    Note: This should be BufferedIOBase in the future.
    https://docs.python.org/3/library/io.html#io.RawIOBase
    """

    def __init__(
        self,
        bucket: str,
        key: str,
        file,
        buffer_size=4096,
        line_terminator=constants.BINARY_NEWLINE,
        writeable: bool = False,
    ):
        assert file.writeable if writeable else True

        self.bucket = bucket
        self.key = key
        self.position = 0
        self.file = file
        self.raw = None
        self.line_terminator = line_terminator
        self.buffer_size = buffer_size
        self.is_writeable = writeable

    def fileno(self) -> int:
        raise OSError(f"fileno not supported for {self.file.identifier}")

    def close(self) -> None:
        """Flush and close this stream"""
        if not self.closed and self.is_writeable:
            self.file.seal()
        self.file = None

    @property
    def size(self) -> int:
        return self.file.size

    @property
    def closed(self) -> bool:
        return self.file is None

    def flush(self):
        if not self.closed and self.is_writeable:
            self.file.seal()

    def isatty(self) -> bool:
        return False

    def readable(self) -> bool:
        return not self.closed

    def writable(self) -> bool:
        return self.file is not None and self.is_writeable

    def checkReadable(self):
        if not self.readable:
            raise OSError("The file is not readable")

    def checkClosed(self):
        if self.closed:
            raise IOError("the file is already closed!")

    def seekable(self) -> bool:
        return True

    def seek(self, offset: int, whence: int = io.SEEK_SET) -> int:
        if whence == io.SEEK_SET:
            self.position = offset
        elif whence == io.SEEK_CUR:
            self.position += offset
        elif whence == io.SEEK_END:
            self.position = self.file.size + offset
        return self.position

    def tell(self) -> int:
        return self.position

    def detach(self):
        raise io.UnsupportedOperation("Operation not supported")

    def read(self, limit: int = -1):
        """
        If 0 bytes are returned, and size was not 0, this indicates end
        of file. If the object is in non-blocking mode and no bytes are
        available, None is returned.
        """
        self.checkClosed()
        self.checkReadable()
        maxcount = self.file.size - self.position
        assert maxcount >= 0
        count = limit
        if limit == 0:
            return b""
        if limit < 0 or limit > maxcount:
            count = maxcount
        buffer = bytearray(count)
        count = self.readinto(buffer)
        if count < len(buffer):
            return buffer[0:count]
        return buffer

    def readinto(self, buffer):
        self.checkReadable()
        if self.closed:
            return -1
        count = self.file.read(buffer, self.position, len(buffer))
        self.position += count
        return count

    def readline(self, limit: int = -1) -> bytes:
        if self.closed:
            return -1

        previous_position = self.position
        print("readline " + limit)
        if limit != -1:
            raise NotImplementedError("limits other than -1 not implemented yet")
        buffer = bytearray(self.buffer_size)
        line = io.BytesIO()

        while True:
            previous_position = self.position
            count = self.readinto(buffer)
            if count == 0:
                break
            index = buffer.find(self.line_terminator, 0)
            if index > 0:
                line.write(buffer[0:index])
                self.position = previous_position + index
                break
            line.write(buffer)
        return line.getvalue()

    def readall(self) -> bytes:
        self.checkClosed()

        length = self.file.size - self.position
        buffer = bytearray(length)
        count = self.readinto(buffer)
        return buffer[0:count]

    def write(self, b):
        self.checkClosed()
        if not self.is_writeable:
            raise IOError("write is not allowed: the file is not writeable!")
        self.file.write(b, self.position, len(b))
        self.position += len(b)
        return len(b)

    def writelines(self, lines: Iterable) -> None:
        self.checkClosed()
        for line in lines:
            self.write(line)
            self.write(self.line_terminator)

    def truncate(self, size=None) -> int:
        self.checkClosed()
        if not self.is_writeable:
            raise IOError("truncate not allowed: the file is not writeable!")
        if size is None:
            size = self.position
        self.file.truncate(size)
        return size


class GEDSInstance(object):
    _lock = threading.Lock()
    _geds: pygeds.GEDS = None
    _known_s3_buckets_lock = threading.Lock()
    _known_s3_buckets: set[str] = set()

    @classmethod
    def init_geds(cls):
        with cls._lock:
            if cls._geds is not None:
                return

            METADATA_SERVER = os.environ.get(
                "GEDS_METADATASERVER",
                f"geds-metadataserver:{pygeds.GEDS.default_metadata_server_port}",
            )
            config = pygeds.GEDSConfig(METADATA_SERVER)
            PORT_VALUE = os.environ.get("GEDS_PORT")
            if PORT_VALUE is not None:
                config.port = int(PORT_VALUE)

            TMP_FOLDER = os.environ.get("GEDS_TMP")
            if TMP_FOLDER is not None:
                config.local_storage_path = TMP_FOLDER

            BLOCK_SIZE = os.environ.get("GEDS_BLOCK_SIZE")
            if BLOCK_SIZE is not None:
                config.cache_block_size = int(BLOCK_SIZE)

            # Init GEDS

            cls._geds = pygeds.GEDS(config)
            try:
                cls._geds.start()
            except status.StatusNotOk as e:
                print(e.status)
                exit(1)
            cls._geds.syncObjectStoreConfigs()

    @classmethod
    def get(cls) -> pygeds.GEDS:
        cls.init_geds()
        return cls._geds

    @classmethod
    def handle_shutdown(cls) -> None:
        if cls._geds != None:
            print("Stopping GEDS --> Spilling data.")
            cls._geds.stop()

    @classmethod
    def register_object_store(
        cls, bucket: str, endpoint_url: str, access_key: str, secret_key: str
    ):
        with cls._known_s3_buckets_lock:
            if bucket in cls._known_s3_buckets:
                return
            geds = cls.get()
            geds.registerObjectStoreConfig(
                bucket=bucket,
                endpointUrl=endpoint_url,
                accessKey=access_key,
                secretKey=secret_key,
            )
            cls._known_s3_buckets.add(bucket)

    @classmethod
    def object_store_mapped(cls, bucket: str) -> bool:
        return bucket in cls._known_s3_buckets

@atexit.register
def handle_shutdown():
    GEDSInstance.handle_shutdown()

def register_object_store(
    bucket: str, endpoint_url: str, access_key: str, secret_key: str
):
    GEDSInstance.register_object_store(bucket, endpoint_url, access_key, secret_key)


def parse_uri(uri: str):
    path = uri.removeprefix("geds://")
    variables = path.split("@")
    bucket, key = path.split("/", maxsplit=1)
    if len(variables) == 3:
        bucket, key = variables[2].split("/", maxsplit=1)

        # Register object store if it does not exist.
        if not GEDSInstance.object_store_mapped(bucket):
            s3_access_key, s3_secret_key = variables[0].split(":")
            s3_host = variables[1]
            if s3_host.endswith(":80"):
                s3_host = s3_host.removesuffix(":80")
                s3_host = f"http://{s3_host}"
            elif s3_host.endswith(":443"):
                s3_host = s3_host.removesuffix(":443")
                s3_host = f"https://{s3_host}"
            else:
                s3_host = f"https://{s3_host}"
            register_object_store(bucket, s3_host, s3_access_key, s3_secret_key)
    elif len(variables) != 1:
        raise NotImplementedError(
            f"unable to parse uri: {uri} - got {variables} - expected geds://access_key:secret@host@bucket/prefix"
        )
    return dict(scheme=SCHEME, bucket=bucket, key=key, client=GEDSInstance.get())


def open(bucket: str, key: str, mode: str, client=None):
    if mode not in constants.BINARY_MODES:
        raise NotImplementedError(
            "bad mode: %r expected one of %r" % (mode, constants.BINARY_MODES)
        )

    f = None
    if mode == constants.READ_BINARY:
        f = client.open(bucket, key)
    elif mode == constants.WRITE_BINARY:
        try:
            f = client.open(bucket, key)
        except:
            f = client.create(bucket, key)
    else:
        raise ValueError(f"Invalid argument for mode: {mode}")
    return GEDSRawInputBase(
        bucket=bucket, key=key, file=f, writeable=mode == constants.WRITE_BINARY
    )


def open_uri(uri_as_str: str, mode: str, transport_params):
    """Return a file-like object pointing to the URI.

    Parameters:

    uri_as_str: str
        The URI to open
    mode: str
        Either "rb" or "wb".  You don't need to implement text modes,
        `smart_open` does that for you, outside of the transport layer.
    transport_params: dict
        Any additional parameters to pass to the `open` function (see below).

    """
    parsed_uri = parse_uri(uri_as_str)
    kwargs = smart_open.utils.check_kwargs(open, transport_params)
    return open(
        parsed_uri["bucket"],
        parsed_uri["key"],
        mode,
        client=parsed_uri["client"],
        **kwargs,
    )
