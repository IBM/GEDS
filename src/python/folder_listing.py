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

S3_ENDPOINT = os.environ.get("S3_ENDPOINT")
S3_ACCESS_KEY = os.environ.get("S3_ACCESS_KEY")
S3_SECRET_KEY = os.environ.get("S3_SECRET_KEY")
S3_BUCKET = os.environ.get("S3_BUCKET")

instance.registerObjectStoreConfig(S3_BUCKET, S3_ENDPOINT, S3_ACCESS_KEY, S3_SECRET_KEY)

folder = instance.status(S3_BUCKET, "folder")
print(folder)
file = instance.open(S3_BUCKET, "folder/testfile.txt")
folder = instance.status(S3_BUCKET, "folder")
print(folder)
