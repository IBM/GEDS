#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

import os
from time import sleep

from pygeds import status, GEDS, GEDSConfig

METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:4381")

instance = GEDS(GEDSConfig(METADATA_SERVER))
try:
    instance.start()
except status.StatusNotOk as e:
    print(e.status)
    exit(1)

try:
    instance.create_bucket("bucket")
except status.StatusNotOk as e:
    pass
file = instance.create("bucket", "testfile")

# Write raw buffer
message = """Hello World
Newline
Newline
end
"""

# Read raw buffer
file.write(bytearray(message, "utf-8"), 0, len(message))
message_read = bytearray(len(message))
l = file.read(message_read, 0, len(message_read))
print(f"Read: {message_read.decode()}")

# Print path
# print(f"File: {file.path()}")

# Seal file

file.seal()

sleep(1000000)
