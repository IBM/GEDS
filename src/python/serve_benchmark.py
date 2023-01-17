#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

import os
import numpy as np
from time import sleep

from pygeds import status, GEDS

METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:50051")
BUCKET_NAME = os.environ.get("BUCKET_NAME", "benchmark")
MAX_THREADS = int(os.environ.get("MAX_THREADS", "16"))
MAX_SIZE = int(os.environ.get("MAX_SIZE", "18"))

instance = GEDS(METADATA_SERVER)
try:
    instance.start()
except status.StatusNotOk as e:
    print(e.status)
    exit(1)

try:
    instance.create_bucket(BUCKET_NAME)
except status.StatusNotOk as e:
    pass

MAX_BUFFERSIZE = 2097152
FACTOR_SIZE = 1024
for i in range(0, MAX_SIZE + 1):
    print(f"Create factor: {i}")
    files = [
        instance.create(BUCKET_NAME, f"{i}-{j}.data") for j in range(0, MAX_THREADS)
    ]
    factor = 2**i
    target_size = factor * FACTOR_SIZE
    position = 0
    while position < target_size:
        buffer = np.random.randint(0, high=255, size=MAX_BUFFERSIZE, dtype=np.uint8)
        s = target_size - position
        if s > MAX_BUFFERSIZE:
            for file in files:
                file.write(buffer, position, MAX_BUFFERSIZE)
            position += MAX_BUFFERSIZE
        else:
            for file in files:
                file.write(buffer, position, MAX_BUFFERSIZE)
            position += s
    for file in files:
        file.seal()
print(
    f"Created all benchmark files with max size {MAX_SIZE} for {MAX_THREADS} threads. Waiting."
)

sleep(1000000)
