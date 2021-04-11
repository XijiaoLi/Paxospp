#ifndef KVSTORE_CLIENT_H
#define KVSTORE_CLIENT_H

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <cstdlib> // int64_t
#include <unistd.h> // usleep
#include <stdlib.h>
#include "paxos.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "kvstore.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using kvstore::KVStore;
using kvstore::KVRequest;
using kvstore::KVResponse;

namespace kvstore {

class KVStoreClient {
  public:
    KVStoreClient(std::shared_ptr<grpc::Channel> channel);
    std::tuple<std::string, std::string> Put(const std::string& key, const std::string& value);
    std::tuple<std::string, std::string> Get(const std::string& key, const std::string& value);

  private:
    std::unique_ptr<KVStore::Stub> stub;
};

} // namespace kvstore

#endif
