/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <thread>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include "GRPCServer.h"
#include "Ports.h"
#include "PubSubMQTT.h"

ABSL_FLAG(std::string, address, "localhost", "Server interface address.");
ABSL_FLAG(uint16_t, port, defaultMetdataServerPort, "Port.");

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  auto serverAddress = FLAGS_address.CurrentValue() + ":" + FLAGS_port.CurrentValue();
 
  std::thread thread_geds([serverAddress]{
  GRPCServer service(serverAddress);
  auto status = service.startAndWait();
  if (!status.ok()) {
    std::cerr << status.message() << std::endl;
  }
  });

  mqtt::async_client_ptr client_ptr = createClient("172.24.33.70:1883",
                                                   "mds_server");
  std::string nodeType = "server";
  mqtt::async_client_ptr connected_client_ptr = connectClient(client_ptr, nodeType);
  mqtt::async_client_ptr subscribed_client_ptr = subscribe(connected_client_ptr,
                                                           "bucket/#", 1);

  std::thread thread_mqtt([connected_client_ptr, subscribed_client_ptr]{
    while (true) {
    auto msg = consumeMessage(subscribed_client_ptr);
    std::string topic = std::get<0>(msg);
    std::string payload = std::get<1>(msg);
    if (payload.empty()){
        break;
    }
    publishData(connected_client_ptr, topic, 1, "Hello World");
    } 
  });

  thread_geds.join();
  thread_mqtt.join();
  return 0;
}
