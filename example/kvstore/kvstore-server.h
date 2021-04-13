/**
 *  @file   kvstore-server.h
 *  @brief  KVStoreServer Interface
 *  This file contains the prototypes for the KVStoreServer class
 *  and some strucures that you will need.
 *  @author Xijiao Li
 *  @date   2021-04-12
 ***********************************************/

#ifndef KVSTORE_SERVER_H
#define KVSTORE_SERVER_H

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
 * \struct Response
 * \brief A structure to represent a Response
 */
struct Response {
  int64_t timestamp;
  std::string err;
  std::string value;
};

/**
 * \struct Op
 * \brief A structure to represent an Op (operation)
 */
struct Op {
  int64_t timestamp;
  int64_t client_id;
  std::string type;
  std::string key;
  std::string value;
};

/**
 * \class KVStoreServer
 * \brief A class for the server side implementation KVStoreServer based on Paxos.
 */
class KVStoreServer final : public KVStore::Service {

  public:

    /// Constructor for KVStoreServer
    KVStoreServer(std::map<std::string, std::string> db_seeds, std::vector<std::string> peers_addr, int me);

    /// KVStore Get service to get a instance and put the value in the response
    grpc::Status Get(ServerContext* context, const KVRequest* request, KVResponse* response) override;

    /// KVStore Put service to put an instance and put the statud in the response
    grpc::Status Put(ServerContext* context, const KVRequest* request, KVResponse* response) override;

  private:
    /// Write a log for the given operation op by making a consensus with its paxos' peers
    std::tuple<std::string, std::string> write_log(Op op);

    /// Excute the given operation op (either a Put or a Get)
    std::tuple<std::string, std::string> execute_log(Op op);

    /// Get the operation according to the given seq number from the log
    Op get_log(int seq);

    paxos::Paxos px; /**< The paxos instance that binds with this KVStore server */
    int committed_seq; /**< The sequence number used to record the latest committed seq */
    mutable std::shared_mutex mu; /**< The mutex used to ensure concurrency */
    std::map<std::string, std::string> db; /**< The local database */
    std::map<int64_t, Response*> latest_requests; /**< The table used to store latest request for each client */
};

} // namespace kvstore

#endif
