/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache2.0
 */
 
/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
// Source:
// https://github.com/grpc/grpc/blob/v1.45.0/examples/cpp/helloworld/greeter_server.cc

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using HelloWorld::Greeter;
using HelloWorld::HelloReply;
using HelloWorld::HelloRequest;

class GreeterClient {
public:
  GreeterClient(std::shared_ptr<Channel> channel) : stub_(Greeter::NewStub(channel)) {}

  std::string SayHello(const std::string &user) {
    HelloRequest request;
    request.set_name(user);

    HelloReply reply;

    ClientContext context;

    Status status = stub_->SayHello(&context, request, &reply);

    if (status.ok()) {
      return reply.message();
    }
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return "RPC failed";
  }

private:
  std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char **argv) {

  std::string target_str = "localhost:50051";
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cerr << "Expected '--target=hostname' as argument." << std::endl;
        return -1;
      }
    } else {
      std::cerr << "Unexpected argument. Supported: '--target=hostname'." << std::endl;
      return -1;
    }
  }

  GreeterClient greeter(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string user("world");
  std::string reply = greeter.SayHello(user);
  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}
