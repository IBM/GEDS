#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

import os
import random
import string

from smart_open import open
import geds_smart_open

S3_ENDPOINT = os.environ.get("S3_ENDPOINT")
S3_ACCESS_KEY = os.environ.get("S3_ACCESS_KEY")
S3_SECRET_KEY = os.environ.get("S3_SECRET_KEY")
S3_BUCKET = os.environ.get("S3_BUCKET")

with open(f"geds://{S3_ACCESS_KEY}:{S3_SECRET_KEY}@{S3_ENDPOINT}:80@{S3_BUCKET}/testfile.txt", 'r') as f:
    for line in f.readlines():
        print(line)

characters = string.digits + string.ascii_letters + string.punctuation + ' '
lines = [''.join(random.choice(characters) for _ in range(random.randint(0, 10)))+'\n' for k in range(0, 10)]
with open("geds://python-test/test.txt", 'w') as f:
    for line in lines:
        f.write(line)
with open("geds://python-test/test.txt", 'r') as f:
    for a, b in zip(lines, f.readlines()):
        assert(a == b)

with open("geds://python-test/test.txt", 'wb') as f:
    f.truncate(0)
    assert(f.size == 0)
