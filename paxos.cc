#include <cstdlib>
#include <string>

#include "paxos.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using paxos::Paxos;
using paxos::EProposal;
using paxos::EResponse;


Status PaxosServiceImpl::Echo(ServerContext* context,
                              const EProposal* proposal,
                              EResponse* response)
  {
    int seq = proposal->seq();
    std::string value = proposal->value();

    response->set_seq(seq);
    response->set_value(value);

    return Status::OK;
  }

  // Status Initialize() {
    // Status get_status = GetCoordinator();
    // auto* coordinator_stub = paxos_stubs_map_->GetCoordinatorStub();
    // // If not successful, start an election for Coordinators.
    // if (!get_status.ok()) {
    //   Status elect_status = ElectNewCoordinator();
    //   if (!elect_status.ok()) {
    //     return Status(
    //         grpc::StatusCode::ABORTED,
    //         "ElectNewCoordinator Failed: " + elect_status.error_message());
    //   }
    // }
    // Status recover_status = GetRecovery();
    // if (!recover_status.ok()) {
    //   return Status(grpc::StatusCode::ABORTED,
    //                 "GetRecovery Failed: " + recover_status.error_message());
    // }
    // return Status::OK;
  // }

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  PaxosServiceImpl service;
  ServerBuilder builder;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
