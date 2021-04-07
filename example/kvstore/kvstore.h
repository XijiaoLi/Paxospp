#ifndef KV_STORE_H
#define KV_STORE_H

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <cstdlib> // std::int64_t
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
  std::int64_t timestamp;
	std::string err;
	std::string value;
};

struct Op {
  std::int64_t timestamp;
  std::int64_t client_id;
  std::string type;
  std::string key;
  std::string value;
};

class KVStoreImpl final : public KVStore::Service {
  public:

    KVStoreImpl(std::map<std::string, std::string> db_seeds);

    grpc::Status Get(ServerContext* context, const KVRequest* request, KVResponse* response) override;

    grpc::Status Put(ServerContext* context, const KVRequest* request, KVResponse* response) override;

    // tell the server to shut itself down
    // void Kill(int seq, std::string v);

  private:

    // Instance* get_instance(int seq);
    // std::tuple<std::string, std::string> write_log(Op op);
    // std::tuple<std::string, std::string> execute_log(Op op);
    // Op get_log(int seq);
    // void auto_update();
    //
    PaxosServiceImpl* px;
    // int me; // me
    // int committed_seq;
    // mutable std::shared_mutex mu; // mu
    std::map<std::string, std::string> db;
    // std::map<std::int64_t, Response*> latest_requests;

};

} // namespace kvstore 

#endif
