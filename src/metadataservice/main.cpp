/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include "GRPCServer.h"
#include "Ports.h"

ABSL_FLAG(std::string, address, "0.0.0.0", "Server interface address.");
ABSL_FLAG(uint16_t, port, defaultMetdataServerPort, "Port.");

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  auto serverAddress = FLAGS_address.CurrentValue() + ":" + FLAGS_port.CurrentValue();
  GRPCServer service(serverAddress);
  auto status = service.startAndWait();
  if (!status.ok()) {
    std::cerr << status.message() << std::endl;
    exit(1);
  }
  return 0;
}
