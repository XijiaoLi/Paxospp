#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>

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


std::vector<std::unique_ptr<grpc::Server>> make(int replica_size, const std::string& addr_list[])
{
  std::map<std::string, std::unique_ptr<Paxos::Stub>> peers;
  std::vector<std::unique_ptr<grpc::Server>> pxos;

  for (int i = 0; i < replica_size; ++i) {
    peers[i] = std::make_unique<Paxos::Stub>(
        grpc::CreateChannel(addr_list[i], grpc::InsecureChannelCredentials())
    );
    std::cout << "Adding " << addr_list[i] << " to the Paxos stubs list ..." << std::endl;
  }

  for (int i = 0; i < replica_size; ++i) {
    PaxosServiceImpl paxos_service(&peers, addr_list[i]);
    std::unique_ptr<grpc::Server> paxos_server = initialize_service(addr_list[i], &paxos_service);
    pxos.pushback(paxos_server);
  }

  return pxos;
}

int main(int argc, char** argv) {
  // random number seed
  srand(time(nullptr));

  // parameters
  int replica_size = 3;
  std::string addr_list[] = {"0.0.0.0:90001", "0.0.0.0:90002", "0.0.0.0:90003"};

  std::vector<std::unique_ptr<grpc::Server>> pxos = make(replica_size, addr_list);

  return 0;
}
