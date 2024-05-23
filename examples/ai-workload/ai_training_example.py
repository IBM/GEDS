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

output_csv = open('file://ai_training.csv', 'w')
output_csv.write("Threads,Time,Data Read,Data Written,Data Spilled\n")

def read_file(tid, buffers):
    path = f'geds://{bucket}/fake_ml_model_{tid}.bin'
    with open(path, 'rb') as f:
        buffers[tid] = f.read()

def write_checkpoints(tid, buffers):
    path = f'geds://{bucket}/checkpoint/checkpoint_{tid}.bin'
    with open(path, 'wb') as f:
        f.write(buffers[tid])

def persist():
    geds_smart_open.relocate()

def benchmark_threads(num_threads, csv):
    buffers = [None]*num_threads
    # Read checkpoint
    start_time = time.time_ns()
    threads = [Thread(target=read_file, args=(i, buffers)) for i in range(0, num_threads)]
    [t.start() for t in threads]
    [t.join() for t in threads]
    duration = (time.time_ns() - start_time) / (1000**3)
    length = sum([len(b) for b in buffers])
    print(f'{num_threads}: Reading {length} took {duration}')
    csv.write(f'{num_threads},{duration},{length},0,0\n')

    # Write checkpoints
    start_time = time.time_ns()
    threads = [Thread(target=write_checkpoints, args=(i, buffers)) for i in range(0, num_threads)]
    [t.start() for t in threads]
    [t.join() for t in threads]
    duration = (time.time_ns() - start_time) / (1000**3)
    print(f'{num_threads}: Writing {length} took {duration}')
    csv.write(f'{num_threads},{duration},0,{length},0\n')
    buffers = None
    start_time = time.time_ns()
    persist()
    duration = (time.time_ns() - start_time) / (1000**3)
    print(f'{num_threads}: Relocating {length} took {duration}')
    csv.write(f'{num_threads},{duration},0,0,{length}\n')

for t in [1,2, 4, 6]:
    benchmark_threads(t, output_csv)
    output_csv.flush()

output_csv.close()
