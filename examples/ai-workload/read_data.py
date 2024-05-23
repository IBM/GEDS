import os
import sys
import time
import io
import numpy as np
from threading import Thread


from smart_open import open
import geds_smart_open
from geds_smart_open import GEDS

bucket = 'geds-test'

folder_in = '100g'
folder_out = 'output/terasort'

AWS_SECRET_ACCESS_KEY = os.getenv('AWS_SECRET_ACCESS_KEY')
AWS_ACCESS_KEY_ID = os.getenv('AWS_ACCESS_KEY_ID')
AWS_ENDPOINT_URL = os.getenv('AWS_ENDPOINT_URL')
geds_smart_open.register_object_store(bucket, AWS_ENDPOINT_URL, AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY)

MAX_THREADS = 16
MAX_SIZE = 28 # 256 MB
MAX_SIZE = 32 # 4GB

output_csv = open('file://read_data.csv', 'w')
output_csv.write("Payload Size,Thread Count,Throughput\n")


def read_file(tid, max_threads, num_threads, size):
    path = f'geds://{bucket}/benchmark/{num_threads}_{tid}_{size}'
    #path = f'geds://{bucket}/benchmark/{max_threads}_{tid}_{size}'
    with open(path, 'rb') as f:
        buf = f.read()
    assert(len(buf) == size)

for t in range(0, MAX_THREADS):
    for s in range(1, MAX_SIZE+1):
        size = 2**s
        num_threads = t + 1
        threads = [Thread(target=read_file, args=[i, MAX_THREADS, num_threads, size]) for i in range(0, num_threads)]
        start_time = time.time_ns()
        [t.start() for t in threads]
        [t.join() for t in threads]
        duration = (time.time_ns() - start_time) / (1000**3)
        rate = float(num_threads) * size / duration / (1024**2)
        print(f"{size}, {num_threads}: {rate} MB/s")
        output_csv.write(f"{size},{num_threads},{rate}\n")

output_csv.close()
