#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>
#include "paxos.h"

using google::protobuf::TextFormat;
using paxos::Paxos;

std::unique_ptr<grpc::Server> initialize_service(const std::string& server_address, grpc::Service* service)
{
  grpc::ServerBuilder builder;
  // listen on the given address
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // register "service" as the instance to communicate with clients; it will corresponds to an *synchronous* service
  builder.RegisterService(service);
  // assemble the server
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  // wait for the server to shutdown
  server->Wait();

  std::cout << "Paxos is now listening on: " << server_address << std::endl;
  return std::move(server);
}


// void start_service(grpc::Server* server)
// {
//   server->Wait();
// }


std::vector<std::unique_ptr<Paxos::Stub>> make_stubs(int replica_size, const std::vector<std::string>& addr_v)
{
  std::vector<std::unique_ptr<Paxos::Stub>> peers; // a list of stubs

  for (int i = 0; i < replica_size; ++i) {
    // at each endpoint, create a channel for paxos to send rpc
    // and create a stub associated with it
    std::unique_ptr<Paxos::Stub> peer_i = std::make_unique<Paxos::Stub>(
      grpc::CreateChannel(addr_v[i], grpc::InsecureChannelCredentials())
    );
    peers.push_back(std::move(peer_i));
    std::cout << "Adding " << addr_v[i] << " to the Paxos stubs list ..." << std::endl;
  }

  return peers;
}

// std::vector<PaxosServiceImpl> make_servers(int replica_size, const std::vector<std::string>& addr_v, std::vector<std::unique_ptr<Paxos::Stub>>* peers)
// {
//   std::vector<PaxosServiceImpl> pxos; // a list of paxos service
//
//   for (int i = 0; i < replica_size; ++i) {
//     PaxosServiceImpl paxos_service(peers, i);
//     std::unique_ptr<grpc::Server> paxos_server = initialize_service(addr_v[i], &paxos_service);
//     pxos.push_back(std::move(paxos_service));
//   }
//
//   return pxos;
// }

int main(int argc, char** argv) {
  // random number seed
  srand(time(nullptr));

  // parameters
  int replica_size = 3;

  std::vector<std::string> addr_v {"0.0.0.0:90001", "0.0.0.0:90002", "0.0.0.0:90003"};

  std::vector<std::unique_ptr<Paxos::Stub>> peers = make_stubs(replica_size, addr_v);

  // std::vector<PaxosServiceImpl> pxos = make_servers(replica_size, addr_v, &peers);

  // PaxosServiceImpl main_server = pxos[0];

   // grpc::Status put_status = main_server.Run(1, "put");

  return 0;
}
