#include <chrono>
#include <memory>
#include <string> // std::string
#include <thread> // std::thread
#include <unordered_map>
#include <utility>
#include <vector>
#include <unistd.h>
#include <random>
#include <assert.h>


#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>
#include "../include/paxos.h"

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
int make_paxos_services(int replica_size, const std::vector<std::string>& addr_v, std::vector<std::shared_ptr<grpc::Channel>> channels)
{
  std::vector<std::unique_ptr<PaxosServiceImpl>> pxs;
  std::vector<std::thread> pxs_threads;
  std::vector<std::unique_ptr<grpc::Server>> servers;
  for (int i = 0; i < replica_size; ++i) {
    std::cout << i << " start" << std::endl;
    std::unique_ptr<PaxosServiceImpl> paxos_service = std::unique_ptr<PaxosServiceImpl>(new PaxosServiceImpl(replica_size,channels, i));
    std::unique_ptr<grpc::Server> paxos_server = initialize_service(addr_v[i], paxos_service.get());
    std::thread paxos_thread(start_service, paxos_server.get()); // If not stored, paxos_server will be destructed every time after the loop
    pxs_threads.push_back(std::move(paxos_thread));
    pxs.push_back(std::move(paxos_service)); 
    servers.push_back(std::move(paxos_server));
    std::cout << i << " ok" << std::endl;
  }

  grpc::Status put_status = pxs.at(1)->Start(1,"put"); // use at as a reference
  usleep(5000000);
  put_status = pxs.at(2)->Start(2, "get");


  for (int i = 0; i < replica_size; i++) {
    pxs_threads[i].join();
    std::cout << i << " joined" << std::endl;
  }

  std::cout << "finished" << std::endl;
  return 0;


}

std::string random_number(){
  // random number seed
  srand(time(nullptr));

  auto randomNum = rand();
  return std::to_string(randomNum);

}

int test_basic_put(){
   // random number seed
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> distribution(0, 500000);
  auto random = std::bind(distribution, generator);
  
  // parameters
  int replica_size = 3;

  std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };
  std::vector<std::shared_ptr<grpc::Channel>> channels = make_channels(replica_size, addr_v);
  // ----------------------  manutal creation of the paxos service----------------------
  PaxosServiceImpl paxos_0(replica_size, channels, 0);
  std::unique_ptr<grpc::Server> paxos_server_0 = initialize_service(addr_v[0], &paxos_0);
  std::thread paxos_thread_0(start_service, paxos_server_0.get());

  PaxosServiceImpl paxos_1(replica_size, channels, 1);
  std::unique_ptr<grpc::Server> paxos_server_1 = initialize_service(addr_v[1], &paxos_1);
  std::thread paxos_thread_1(start_service, paxos_server_1.get());

  PaxosServiceImpl paxos_2(replica_size, channels, 2);
  std::unique_ptr<grpc::Server> paxos_server_2 = initialize_service(addr_v[2], &paxos_2);
  std::thread paxos_thread_2(start_service, paxos_server_2.get());

  std::string num1 = std::to_string(random());
  std::string num2 = std::to_string(random());

  std::cout << "num1:" << num1 << " num2:" << num2 << std::endl;
  grpc::Status put_status = paxos_1.Start(1, num1);
  put_status = paxos_2.Start(2, num2);
  put_status = paxos_2.Start(2, std::to_string(random()));

  bool decided0, decided1, decided2; 
  std::string val0, val1, val2;
  paxos_0.status(decided0,val0,2);
  paxos_1.status(decided1,val1,2);
  paxos_2.status(decided2,val2,2);

  assert(decided0 == decided1 && val0 == val1 && "s0 and s1 agree");
  assert(decided1 == decided2 && val1 == val2 && "s1 and s2 agree");
  std::cout << "=============" << val0 << " is decided among three servers\n";
  
  paxos_thread_0.join();
  paxos_thread_1.join();
  paxos_thread_2.join();
  usleep(200000);
  
  return 0;

}

int main(int argc, char** argv) {
  
  test_basic_put();
  

  // ----------------------  auto/loop creation of the paxos service----------------------  
  // make_paxos_services(replica_size, addr_v, channels);

  
}
