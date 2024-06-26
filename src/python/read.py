#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

import os

from pygeds import status, GEDS, GEDSConfig

METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:4381")
instance = GEDS(GEDSConfig(METADATA_SERVER))
try:
    instance.start()
except status.StatusNotOk as e:
    print(e.status)
    exit(1)

file = instance.open("bucket", "testfile")

buffer = bytearray(file.size)
l = file.read(buffer, 1, len(buffer))
print(f"Read {l} bytes")
print(buffer.decode("utf-8"))
