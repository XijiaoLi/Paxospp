#ifndef KV_STORE_H
#define KV_STORE_H

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

class KVStoreImpl final : public KVStore::Service {
  public:

    KVStoreImpl(std::map<std::string, std::string> db_seeds, int peers_num, std::vector<std::string> peers_addr, int me);

    grpc::Status Get(ServerContext* context, const KVRequest* request, KVResponse* response) override;

    grpc::Status Put(ServerContext* context, const KVRequest* request, KVResponse* response) override;

    // tell the server to shut itself down
    // void Kill(int seq, std::string v);

  private:

    // Instance* get_instance(int seq);
    std::tuple<std::string, std::string> write_log(Op op);
    std::tuple<std::string, std::string> execute_log(Op op);
    Op get_log(int seq, Op op);
    // void auto_update();
    //
    PaxosServiceImpl px;
    PaxosServiceImpl px1;
    PaxosServiceImpl px2;
    // int me; // me
    int committed_seq;
    mutable std::shared_mutex mu; // mu
    std::map<std::string, std::string> db;
    std::map<int64_t, Response*> latest_requests;

};

} // namespace kvstore

#endif
