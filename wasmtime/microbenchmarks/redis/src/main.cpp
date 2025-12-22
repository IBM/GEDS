#include <hiredis/hiredis.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <random>

std::string random_string(const size_t length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> distribution(0, charset.size() - 1);

    std::string random_str;
    for (size_t i = 0; i < length; ++i) {
        random_str += charset[distribution(rng)];
    }
    return random_str;
}

double benchmark_set(redisContext *context, const std::vector<std::pair<std::string, std::string> > &kv_pairs) {
    auto const start = std::chrono::high_resolution_clock::now();

    for (const auto &[key, value]: kv_pairs) {
        auto *reply = static_cast<redisReply *>(redisCommand(context, "SET %s %s", key.c_str(), value.c_str()));
        if (reply == nullptr) {
            std::cerr << "Failed to write key: " << key << "\n";
            exit(EXIT_FAILURE);
        }
        freeReplyObject(reply);
    }

    auto const end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

double benchmark_get(redisContext *context, const std::vector<std::pair<std::string, std::string> > &kv_pairs) {
    auto const start = std::chrono::high_resolution_clock::now();

    for (const auto &[key, _]: kv_pairs) {
        auto *reply = static_cast<redisReply *>(redisCommand(context, "GET %s", key.c_str()));
        if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
            std::cerr << "Failed to read key: " << key << "\n";
            freeReplyObject(reply);
            continue;
        }
        freeReplyObject(reply);
    }

    auto const end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <address> <port> <num_pairs> <key_size> <value_size>\n";
        return 1;
    }

    const std::string address = argv[1];
    const int port = std::stoi(argv[2]);
    const int num_pairs = std::stoi(argv[3]);
    const int key_size = std::stoi(argv[4]);
    const int value_size = std::stoi(argv[5]);

    std::cout << "---------------[Redis microbenchmark]---------------\n";
    std::cout << "[Address]: " << address << "\n";
    std::cout << "[Port]: " << port << "\n";
    std::cout << "[Number of K/V pairs]: " << num_pairs << "\n";
    std::cout << "[Key size]: " << key_size << "\n";
    std::cout << "[Value size]: " << value_size << "\n";
    std::cout << "----------------------------------------------------\n";

    redisContext *context = redisConnect(address.c_str(), port);
    if (context == nullptr || context->err) {
        std::cerr << "Error: Unable to connect to Redis \n";
        if (context) {
            std::cerr << "Error: " << context->errstr << "\n";
            redisFree(context);
        }
        return EXIT_FAILURE;
    }

    std::vector<std::pair<std::string, std::string> > kv_pairs;
    kv_pairs.reserve(num_pairs);
    for (int i = 0; i < num_pairs; ++i) {
        kv_pairs.emplace_back(random_string(key_size), random_string(value_size));
    }

    std::cout << "Performing SET microbenchmark...\n";
    const double set_duration = benchmark_set(context, kv_pairs);
    std::cout << "Average SET time per key/value pair: " << set_duration / num_pairs << " ms\n";

    std::cout << "\nPerforming GET microbenchmark...\n";
    const double get_duration = benchmark_get(context, kv_pairs);
    std::cout << "Average GET time per key/value pair: " << get_duration / num_pairs << " ms\n";

    redisFree(context);
    return EXIT_SUCCESS;
}
