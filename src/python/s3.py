#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

import os
import numpy as np
from time import sleep

from pygeds import status, GEDS, GEDSConfig

METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:4381")
instance = GEDS(GEDSConfig(METADATA_SERVER))
try:
    instance.start()
except status.StatusNotOk as e:
    print(e.status)
    exit(1)

S3_ENDPOINT = os.environ.get("S3_ENDPOINT")
S3_ACCESS_KEY = os.environ.get("S3_ACCESS_KEY")
S3_SECRET_KEY = os.environ.get("S3_SECRET_KEY")
S3_BUCKET = os.environ.get("S3_BUCKET")

instance.registerObjectStoreConfig(S3_BUCKET, S3_ENDPOINT, S3_ACCESS_KEY, S3_SECRET_KEY)

folder = "folder/"
files = instance.list_folder(S3_BUCKET, folder)
print(f'Listing "{S3_BUCKET}/{folder}"')
for file in files:
    print(file)

print("mkdir /")
instance.mkdirs(S3_BUCKET, "/")
print("mkdir ''")
instance.mkdirs(S3_BUCKET, "")
print("mkdirs 'a/b/c/d'")
instance.mkdirs(S3_BUCKET, "a/b/c/d")

print("{}: {}".format("list /", instance.list(S3_BUCKET, "/")))
print("{}: {}".format("list /a", instance.list(S3_BUCKET, "/a")))
print("{}: {}".format("list a", instance.list(S3_BUCKET, "a")))
print("{}: {}".format("list a/b", instance.list(S3_BUCKET, "a/b")))
print("{}: {}".format("list a/b/c", instance.list(S3_BUCKET, "a/b/c")))
print("{}: {}".format("list a/b/c/d", instance.list(S3_BUCKET, "a/b/c/d")))

print("{}: {}".format("list_folder /", instance.list_folder(S3_BUCKET, "/")))
print("{}: {}".format("list_folder /a", instance.list_folder(S3_BUCKET, "/a")))
print("{}: {}".format("list_folder a", instance.list_folder(S3_BUCKET, "a")))
print("{}: {}".format("list_folder a/", instance.list_folder(S3_BUCKET, "a/")))
print("{}: {}".format("list_folder a/b", instance.list_folder(S3_BUCKET, "a/b")))
print("{}: {}".format("list_folder a/b/c", instance.list_folder(S3_BUCKET, "a/b/c")))
print(
    "{}: {}".format("list_folder a/b/c/d", instance.list_folder(S3_BUCKET, "a/b/c/d"))
)

print("Reading a test file")
print(repr(instance.status(S3_BUCKET, "testfile.txt")))
print(repr(instance.status(S3_BUCKET, "folder/")))
print(instance.list_folder(S3_BUCKET, ""))

testfile = instance.open(S3_BUCKET, "testfile.txt")
buffer = np.zeros(30, dtype=np.uint8)
l = testfile.read(buffer, 0, len(buffer))
print(buffer)

print("Read a second time")
l = testfile.read(buffer, 0, len(buffer))
print(buffer)

sleep(10000)
