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
  std::cout << "Paxos is now listening on: " << server_address << std::endl;
  return std::move(server);
}


void start_service(grpc::Server* server)
{
  server->Wait();
}

void make(){}

int main(int argc, char** argv) {
  // random number seed
  srand(time(nullptr));
  // parameters
  replica_size = 5;

  // set server address
  return 0;
}
