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

struct Response {
  int64_t timestamp;
  std::string err;
  std::string value;
};

struct Op {
  int64_t timestamp;
  int64_t client_id;
  std::string type;
  std::string key;
  std::string value;
};

class KVStoreServer final : public KVStore::Service {
  public:
    KVStoreServer(std::map<std::string, std::string> db_seeds, std::vector<std::string> peers_addr, int me);
    grpc::Status Get(ServerContext* context, const KVRequest* request, KVResponse* response) override;
    grpc::Status Put(ServerContext* context, const KVRequest* request, KVResponse* response) override;

  private:
    std::tuple<std::string, std::string> write_log(Op op);
    std::tuple<std::string, std::string> execute_log(Op op);
    Op get_log(int seq);

    PaxosServiceImpl px;
    int committed_seq;
    mutable std::shared_mutex mu;
    std::map<std::string, std::string> db;
    std::map<int64_t, Response*> latest_requests;
};

} // namespace kvstore

#endif
