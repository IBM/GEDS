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

AWS_SECRET_ACCESS_KEY = os.getenv('AWS_SECRET_ACCESS_KEY')
AWS_ACCESS_KEY_ID = os.getenv('AWS_ACCESS_KEY_ID')
AWS_ENDPOINT_URL = os.getenv('AWS_ENDPOINT_URL')
geds_smart_open.register_object_store(bucket, AWS_ENDPOINT_URL, AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY)

MAX_THREADS = 16
MAX_SIZE = 28 # 256 MB
MAX_SIZE = 33 # 8GB

output_csv = open('file://write_data.csv', 'w')
output_csv.write("Payload Size,Thread Count,Throughput\n")


def write_file(tid, nthreads, buffer, size):
    path = f'geds://{bucket}/benchmark/{nthreads}_{tid}_{size}'
    with open(path, 'wb') as f:
        f.write(buffer)
        f.flush()
        f.close()

for t in range(0, MAX_THREADS):
    num_threads = t + 1
    if num_threads > 1 and num_threads % 2 == 1:
        continue
    for s in range(1, MAX_SIZE+1):
        size = 2**s
        buffers = [np.random.randint(0, high=255, size=size, dtype=np.uint8) for i in range(0, num_threads)]
        threads = [Thread(target=write_file, args=[i, num_threads, buffers[i], size]) for i in range(0, num_threads)]
        start_time = time.time_ns()
        [t.start() for t in threads]
        [t.join() for t in threads]
        duration = (time.time_ns() - start_time) / (1000**3)
        rate = float(num_threads) * size / duration / (1024**2)
        print(f"{size}, {num_threads}: {rate} MB/s")
        output_csv.write(f"{size},{num_threads},{rate}\n")
        output_csv.flush()

output_csv.close()
# Keep GEDS open to allow reading data
time.sleep(1000000000)
