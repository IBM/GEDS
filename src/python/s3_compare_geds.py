#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

# Requirements
# - numpy
# - smart_open[s3]

import os
import boto3
import numpy as np
import smart_open
import threading
import random

from time import sleep

from pygeds import status, GEDS

METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:50051")
SIZE = os.environ.get("SIZE", "100")

instance = GEDS(METADATA_SERVER)
try:
    instance.start()
except status.StatusNotOk as e:
    print(e.status)
    exit(1)

S3_ENDPOINT = os.environ.get("S3_ENDPOINT")
S3_ACCESS_KEY = os.environ.get("S3_ACCESS_KEY")
S3_SECRET_KEY = os.environ.get("S3_SECRET_KEY")
S3_BUCKET = os.environ.get("S3_BUCKET")
SIZE = f"sf{SIZE}_parquet"

instance.registerObjectStoreConfig(S3_BUCKET, S3_ENDPOINT, S3_ACCESS_KEY, S3_SECRET_KEY)

boto3_session = boto3.Session(
    aws_access_key_id=S3_ACCESS_KEY,
    aws_secret_access_key=S3_SECRET_KEY,
)
s3_direct_transport_params = {
    "client": boto3_session.client("s3", endpoint_url=S3_ENDPOINT, use_ssl=False)
}


folder = f"{SIZE}/"
files = instance.list(S3_BUCKET, folder)
random.shuffle(files)

print(f"Reading all files in {folder}")

not_matching = []


def open_and_compare_file(filestatus):
    filename = filestatus.name
    if filestatus.isDirectory:
        return
    file = instance.open(S3_BUCKET, filename)
    size = file.size

    geds_data = np.zeros(size, dtype=np.uint8)
    l = file.read(geds_data, 0, len(geds_data))
    assert l == size

    if size == 0:
        return
    with smart_open.open(
        f"s3://{S3_BUCKET}/{filename}",
        "rb",
        transport_params=s3_direct_transport_params,
    ) as s3_file:
        s3_data = np.frombuffer(s3_file.read(), dtype=np.uint8).flatten()
        if (s3_data != geds_data).any():
            loc = np.where((geds_data == s3_data) != True)
            print(f"S3 and GEDS do not match for {filename}: {loc}")
            not_matching.append(filename)


threads = [threading.Thread(target=open_and_compare_file, args=(f,)) for f in files]
[t.start() for t in threads]
[t.join() for t in threads]

print(f"Read and compared all {len(files)} files. Serving data.")
if len(not_matching) > 0:
    for f in not_matching:
        print(f"{f} did not match!")
sleep(100000)
