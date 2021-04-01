#include <chrono>
#include <memory>
#include <string> // std::string
#include <thread> // std::thread
#include <unordered_map>
#include <utility>
#include <vector>

#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>
#include "paxos.h"

using google::protobuf::TextFormat;
using paxos::Paxos;

void square(int x) {
    int accum = x * x;
    std::cout << "acc: " << accum << std::endl;
}

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
  // server->Wait();

  std::cout << "Paxos is now listening on: " << server_address << std::endl;
  return std::move(server);
}


void start_service(grpc::Server* server)
{
  // wait for the server to shutdown
  server->Wait();
  std::cout << "wait for the server to shutdown..." << std::endl;
}


std::vector<std::shared_ptr<grpc::Channel>> make_channels(int replica_size, const std::vector<std::string>& addr_v)
{
  std::vector<std::shared_ptr<grpc::Channel>> channels; // a list of channels

  for (int i = 0; i < replica_size; ++i) {
    // at each endpoint, create a channel for paxos to send rpc
    std::shared_ptr<grpc::Channel> channel_i = grpc::CreateChannel(addr_v[i], grpc::InsecureChannelCredentials());
    channels.push_back(std::move(channel_i));
    std::cout << "Adding " << addr_v[i] << " to the channel list ..." << std::endl;
  }

  return channels;
}


// std::vector<PaxosServiceImpl> make_paxos_services(int replica_size, const std::vector<std::string>& addr_v, std::vector<std::shared_ptr<grpc::Channel>> channels)
// {
//   std::vector<PaxosServiceImpl> pxos; // a list of paxos services
//
//   for (int i = 0; i < replica_size; ++i) {
//     PaxosServiceImpl paxos_service(replica_size, channels, i);
//     // std::unique_ptr<grpc::Server> paxos_server = initialize_service(addr_v[i], &paxos_service);
//     pxos.push_back(std::move(paxos_service));
//   }
//   return pxos;
// }

int main(int argc, char** argv) {
  // random number seed
  srand(time(nullptr));

  // parameters
  int replica_size = 3;

  std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053"};

  std::vector<std::shared_ptr<grpc::Channel>> channels = make_channels(replica_size, addr_v);

  /* not sure why this does not work
  std::vector<PaxosServiceImpl*> pxs;
  std::vector<std::thread> pxs_threads;
  for (int i = 0; i < replica_size; ++i) {
    std::cout << i << " start" << std::endl;
    PaxosServiceImpl paxos_service(replica_size, channels, i);
    std::unique_ptr<grpc::Server> paxos_server = initialize_service(addr_v[i], &paxos_service);
    std::thread paxos_thread(start_service, paxos_server.get());
    pxs_threads.push_back(std::move(paxos_thread));
    pxs.push_back(&paxos_service);
    std::cout << i << " ok" << std::endl;
  }

  for (int i = 0; i < replica_size; i++) {
    pxs_threads[i].join();
    std::cout << i << " joined" << std::endl;
  }

  PaxosServiceImpl main_server = pxos[0];
  */

  PaxosServiceImpl paxos_0(replica_size, channels, 0);
  std::unique_ptr<grpc::Server> paxos_server_0 = initialize_service(addr_v[0], &paxos_0);
  std::thread paxos_thread_0(start_service, paxos_server_0.get());

  PaxosServiceImpl paxos_1(replica_size, channels, 1);
  std::unique_ptr<grpc::Server> paxos_server_1 = initialize_service(addr_v[1], &paxos_1);
  std::thread paxos_thread_1(start_service, paxos_server_1.get());

  PaxosServiceImpl paxos_2(replica_size, channels, 2);
  std::unique_ptr<grpc::Server> paxos_server_2 = initialize_service(addr_v[2], &paxos_2);
  std::thread paxos_thread_2(start_service, paxos_server_2.get());

  grpc::Status put_status = paxos_1.Run(1, "put");
  put_status = paxos_2.Run(1, "get");

  paxos_thread_0.join();
  paxos_thread_1.join();
  paxos_thread_2.join();

  return 0;
}
