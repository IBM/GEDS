#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

# stdlib
import os
import time

# third party
import paho.mqtt.client as mqtt_client

# relative
from pygeds import status, GEDS, GEDSConfig


METADATA_SERVER = os.environ.get("GEDS_METADATASERVER", "zac13:4381")
MQTT: bool = False


def on_message(client, userdata, msg):
    print(f"Received {msg.payload} on topic {msg.topic}")
    file = instance.open("bucket", "testfile")
    time.sleep(.1)
    buffer = bytearray(file.size)
    l = file.read(buffer, 0, len(buffer))
    print(f"Read {l} bytes")
    print(buffer.decode("utf-8"))


instance = GEDS(GEDSConfig(METADATA_SERVER))
try:
    instance.start()
except status.StatusNotOk as e:
    print(e.status)
    exit(1)

file = instance.open("bucket", "testfile")

buffer = bytearray(file.size)
l = file.read(buffer, 1, len(buffer))
print(f"Read {l} bytes")
print(buffer.decode("utf-8"))

if MQTT:
    client = mqtt_client.Client(client_id="client2")
    client.on_message = on_message

    client.connect("172.24.33.70", 1883)
    client.subscribe("bucket/testfile")

    client.loop_forever()
    client.disconnect()
