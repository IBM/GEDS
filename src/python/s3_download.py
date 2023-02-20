#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

import os

from pygeds import status, GEDS

METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:4381")
instance = GEDS(METADATA_SERVER)
try:
    instance.start()
except status.StatusNotOk as e:
    print(e.status)
    exit(1)

S3_BUCKET = "geds-test"

folder = ""
files = instance.list_folder(S3_BUCKET, folder)
print(f'Listing "{S3_BUCKET}/{folder}"')
for file in files:
    print(file)

print("Reading a test file")
testfile = instance.open(S3_BUCKET, "testfile.txt")
buffer = bytearray(30)
l = testfile.read(buffer, 0, len(buffer))
print(buffer.decode("utf-8"))
