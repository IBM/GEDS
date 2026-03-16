/**
 * Copyright 2022- IBM Inc. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mqtt/async_client.h"


// https://github.com/eclipse/paho.mqtt.cpp/issues/141
mqtt::async_client_ptr createClient(std::string serverAddress,
                                    std::string clientID){
    auto client_ptr = std::make_shared<mqtt::async_client>(serverAddress,
                                                           clientID);
    std::cout << "Created MQTT client to " + serverAddress + " and ID " + clientID << std::endl;
    return client_ptr;
}

mqtt::async_client_ptr connectClient(std::shared_ptr<mqtt::async_client> client_ptr,
                                     std::string node){
    auto connOpts = std::make_shared<mqtt::connect_options>();

    if (node == "server"){
        connOpts->set_keep_alive_interval(20);
        connOpts->set_clean_session(false);
        connOpts->set_automatic_reconnect(true);
    }
    if (node == "client"){
        connOpts->set_clean_session(false);
    }
    client_ptr->connect(*connOpts)->wait();
    std::cout << "Connected MQTT client" << std::endl;
    return client_ptr;
}

void publishData(mqtt::async_client_ptr client_ptr,
                 std::string topic,
				 int QoS,
                 std::string data){
	mqtt::topic top(*client_ptr, topic, QoS, true);

    mqtt::message_ptr message = mqtt::make_message(topic, data);
    message->set_qos(QoS);
    
    // mqtt::message message = mqtt::message();
    // message.set_qos(QoS);

    message->set_payload("A single message");
	client_ptr->publish(message);
    std::cout << "Published data to " + topic << std::endl;
}

mqtt::async_client_ptr subscribe(mqtt::async_client_ptr client_ptr,
                                 std::string topic,
                                 int QoS){
    client_ptr->subscribe(topic, QoS)->wait();
    std::cout << "Subscribed to " + topic << std::endl;
    return client_ptr;
}

void unsubscribe(mqtt::async_client_ptr client_ptr,
                 std::string topic){
    client_ptr->unsubscribe(topic)->wait();
    client_ptr->stop_consuming();
    std::cout << "Unsubscribed from " + topic << std::endl;
}

std::tuple<std::string, std::string> consumeMessage(mqtt::async_client_ptr client_ptr){
    client_ptr->start_consuming();
    auto msg = client_ptr->consume_message();
    // msg->get_payload()
    return std::make_tuple(msg->get_topic(), msg->to_string());
}

void disconnectClient(mqtt::async_client_ptr client_ptr){
	client_ptr->disconnect()->wait();
    std::cout << "Disconnected client" << std::endl;
}


// Example publisher
// ---------------------------------------------------------------
// #include "PubSub.h"
//
// int main(int argc, char **argv) {
//   mqtt::async_client_ptr client_ptr = createClient("tcp://localhost:1883",
//                                                    "mds_server");
//   std::string node_type = "server";
//   mqtt::async_client_ptr connected_client_ptr = connectClient(client_ptr,
//                                                               node_type);
//   publishData(connected_client_ptr, "home/file1", 1, "Hello World");
//   disconnectClient(connected_client_ptr);
//   return 0;
// }

// Example subscriber
// ---------------------------------------------------------------
// #include "PubSub.h"
//
// int main(void)
// {
//   mqtt::async_client_ptr client_ptr = createClient("tcp://localhost:1883",
//   				                      "mds_client");
//   std::string node_type = "client";
//   mqtt::async_client_ptr connected_client_ptr = connectClient(client_ptr,
//                                                               node_type);
//   mqtt::async_client_ptr subscribed_client_ptr = subscribe(connected_client_ptr,
//                                                            "home/file1", 1);
//   while (true) {
//     auto msg = consumeMessage(subscribed_client_ptr);
//     std::string topic = std::get<0>(msg);
//     std::string payload = std::get<1>(msg);
//
//     if (payload.empty()){
//         break;
//     }
//     std::cout << topic + " " + payload << std::endl;
//     }
//  	return 0;
// }
