#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

# stdlib
import os
import json
import time

# third party
import paho.mqtt.client as mqtt_client

# relative
from pygeds import status, GEDS, GEDSConfig

METADATA_SERVER: str = os.environ.get("GEDS_METADATASERVER", "zac13:4381")
MQTT: bool = False


if MQTT:
    client = mqtt_client.Client(client_id="client1")
    client.connect("172.24.33.70", 1883)

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

file.seal()
if MQTT:
    client = mqtt_client.Client()
    client.connect("172.24.33.70", 1883)
    client.loop_start()
    client.publish("bucket/testfile", json.dumps({"file_name": "testfile",
                                                 "created": True}).encode())
    client.disconnect()
    client.loop_stop()

file2 = instance.create("bucket2", "testfile2")
file2.write(message_read, 0, len(message))
file2.seal()

# Seal file

sleep(1000000)
