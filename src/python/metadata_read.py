#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

import os

from pygeds import status, GEDS


def get_geds_instance():
    METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:4381")
    instance = GEDS(METADATA_SERVER)
    try:
        instance.start()
    except status.StatusNotOk as e:
        print(e.status)
        exit(1)
    return instance


message = "Hello World!"
geds = get_geds_instance()
testfile = geds.open("metadata", "String")
print("Metadata: "+testfile.metadata)
assert testfile.metadata == "Hello World"

testfile = geds.open("metadata", "ByteArray")
message = bytearray(
    [
        0x54,
        0x8D,
        0x3E,
        0xE9,
        0xE7,
        0x79,
        0x34,
        0x92,
        0x8F,
        0x85,
        0x99,
        0x60,
        0x54,
        0x3E,
        0x7C,
        0x03,
    ]
)
assert testfile.metadata_bytes == message
