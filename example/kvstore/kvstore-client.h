/**
 *  @file   kvstore-client.h
 *  @brief  KVStoreClient Interface
 *  This file contains the prototypes for the KVStoreClient class
 *  and some strucures that you will need.
 *
 *  @author Xijiao Li
 *  @date   2021-04-12
 ***********************************************/

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

/**
 * \class KVStoreClient
 * \brief A class for the Client side implementation KVStoreClient based on Paxos.
 */
class KVStoreClient {
  public:
    // Constructor for KVStoreClient
    KVStoreClient(std::shared_ptr<grpc::Channel> channel);

    /// KVStore Get service to get a instance from the server
    std::tuple<std::string, std::string> Get(const std::string& key, const std::string& value);

    /// KVStore Put service to Put a given key value pair into server's database
    std::tuple<std::string, std::string> Put(const std::string& key, const std::string& value);

  private:
    std::unique_ptr<KVStore::Stub> stub; /**< The stub used to sent rpc message to the server */
};

} // namespace kvstore

#endif
