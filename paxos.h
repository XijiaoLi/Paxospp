#ifndef PAXOS_H
#define PAXOS_H
#endif

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "paxos.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using paxos::Paxos;
using paxos::EProposal;
using paxos::EResponse;

class PaxosServiceImpl final : public Paxos::Service {
 public:
  Status Echo(ServerContext* context,
              const EProposal* s_proposal,
              EResponse* s_response) override;
  // Status Receive(ServerContext* context, const Proposal* proposal,
  //                 Response* response) override;

  // Status Initialize();

 // private:

  // template <typename Request>
  // grpc::Status RunPaxos(const Request& req);
  // grpc::Status GetCoordinator();
  // grpc::Status ElectNewCoordinator();
  // grpc::Status GetRecovery();
  // bool RandomFail();

  // const std::string my_paxos_address_;
  // KeyValueDataBase* kv_db_;
  // PaxosStubsMap* paxos_stubs_map_;
  // std::shared_mutex mu_;
  // double fail_rate_;
};
