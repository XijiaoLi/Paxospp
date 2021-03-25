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


grpc::Status PaxosServiceImpl::Echo(ServerContext* context, const EProposal* proposal, EResponse* response)
{
  int seq = proposal->seq();
  std::string value = proposal->value();

  response->set_seq(seq);
  response->set_value(value);

  return grpc::Status::OK;
}

grpc::Status PaxosServiceImpl::Prepare(ServerContext* context, const Proposal* proposal, Response* response)
{
  // int seq = proposal->seq();
  // std::string value = proposal->value();
  //
  // response->set_seq(seq);
  // response->set_value(value);

  return grpc::Status::OK;
}

grpc::Status PaxosServiceImpl::Accept(ServerContext* context, const Proposal* proposal, Response* response)
{
  // int seq = proposal->seq();
  // std::string value = proposal->value();
  //
  // response->set_seq(seq);
  // response->set_value(value);

  return grpc::Status::OK;
}

grpc::Status PaxosServiceImpl::Decide(ServerContext* context, const Proposal* proposal, Response* response)
{
  // int seq = proposal->seq();
  // std::string value = proposal->value();
  //
  // response->set_seq(seq);
  // response->set_value(value);

  return grpc::Status::OK;
}

grpc::Status PaxosServiceImpl::Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response)
{
  return grpc::Status::OK;
}


  // grpc::Status Initialize() {
  //   grpc::Status get_status = GetCoordinator();
  //   auto* coordinator_stub = paxos_stubs_map_->GetCoordinatorStub();
  //   // If not successful, start an election for Coordinators.
  //   if (!get_status.ok()) {
  //     grpc::Status elect_status = ElectNewCoordinator();
  //     if (!elect_status.ok()) {
  //       return grpc::Status(
  //           grpc::StatusCode::ABORTED,
  //           "ElectNewCoordinator Failed: " + elect_status.error_message());
  //     }
  //   }
  //   grpc::Status recover_status = GetRecovery();
  //   if (!recover_status.ok()) {
  //     return grpc::Status(grpc::StatusCode::ABORTED,
  //                   "GetRecovery Failed: " + recover_status.error_message());
  //   }
  //   return grpc::Status::OK;
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
