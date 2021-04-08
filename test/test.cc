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

  std::cout << "Paxos is now listening on: " << server_address << std::endl;
  return std::move(server);
}

void shutdown_service(grpc::Server* server){
  server->Shutdown();
  std::cout << "wait for the server to shutdown..." << std::endl;
}


void start_service(grpc::Server* server)
{
  // wait for the server to start
  server->Wait();
  std::cout << "wait for the server to start..." << std::endl;
}


std::vector<std::shared_ptr<grpc::Channel>> make_channels(int peers_num, const std::vector<std::string>& addr_v)
{
  std::vector<std::shared_ptr<grpc::Channel>> channels; // a list of channels

  for (int i = 0; i < peers_num; ++i) {
    // at each endpoint, create a channel for paxos to send rpc
    std::shared_ptr<grpc::Channel> channel_i = grpc::CreateChannel(addr_v[i], grpc::InsecureChannelCredentials());
    channels.push_back(std::move(channel_i));
    std::cout << "Adding " << addr_v[i] << " to the channel list ..." << std::endl;
  }

  return channels;
}


void test_heavy_put(int peers_num, const std::vector<std::string>& addr_v, std::vector<std::shared_ptr<grpc::Channel>> channels, int put_size)
{
  std::vector<std::unique_ptr<PaxosServiceImpl>> pxs;
  std::vector<std::thread> pxs_threads;
  std::vector<std::unique_ptr<grpc::Server>> servers;

  for (int i = 0; i < peers_num; ++i) {
    std::cout << i << " start" << std::endl;
    std::unique_ptr<PaxosServiceImpl> paxos_service = std::unique_ptr<PaxosServiceImpl>(
      new PaxosServiceImpl(peers_num,channels, i)
    );
    std::unique_ptr<grpc::Server> paxos_server = initialize_service(addr_v[i], paxos_service.get());
    std::thread paxos_thread(start_service, paxos_server.get()); // If not stored, paxos_server will be destructed every time after the loop
    pxs_threads.push_back(std::move(paxos_thread));
    pxs.push_back(std::move(paxos_service));
    servers.push_back(std::move(paxos_server));
    std::cout << i << " ok" << std::endl;
  }

  // random number seed
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> distribution(0, 500000);
  auto random = std::bind(distribution, generator);

  // heavy put
  for (int i = 0; i < put_size; i++){
    int server_num = random()%peers_num;
    grpc::Status put_status = pxs.at(server_num)->Start(i, std::to_string(random()));
  }

  // check all saved values
  for (int i = 0; i < put_size; i++){
    auto [ decided_0, val_0 ] = pxs.at(0)->Status(i);
    for (int j = 1; j < peers_num; j++){
      auto [ decided_1, val_1 ] = pxs.at(j)->Status(i);
      assert(decided_0 && decided_1 && val_0 == val_1 );
    }
  }

  // shut down
  for (int i = 0; i < peers_num; i++) {
    shutdown_service(servers.at(i).get());
  }

  // continue to listen
  for (int i = 0; i < peers_num; i++) {
    pxs_threads[i].join();
    std::cout << i << " joined" << std::endl;
  }

  std::cout << "----------------Test_heavy_put: Passed" << std::endl;
}


void test_basic_put(){
   // random number seed
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> distribution(0, 500000);
  auto random = std::bind(distribution, generator);

  // parameters
  int peers_num = 3;

  std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };
  std::vector<std::shared_ptr<grpc::Channel>> channels = make_channels(peers_num, addr_v);
  // ----------------------  manutal creation of the paxos service----------------------
  PaxosServiceImpl paxos_0(peers_num, channels, 0);
  std::unique_ptr<grpc::Server> paxos_server_0 = initialize_service(addr_v[0], &paxos_0);
  std::thread paxos_thread_0(start_service, paxos_server_0.get());

  PaxosServiceImpl paxos_1(peers_num, channels, 1);
  std::unique_ptr<grpc::Server> paxos_server_1 = initialize_service(addr_v[1], &paxos_1);
  std::thread paxos_thread_1(start_service, paxos_server_1.get());

  PaxosServiceImpl paxos_2(peers_num, channels, 2);
  std::unique_ptr<grpc::Server> paxos_server_2 = initialize_service(addr_v[2], &paxos_2);
  std::thread paxos_thread_2(start_service, paxos_server_2.get());

  std::string num1 = std::to_string(random());
  std::string num2 = std::to_string(random());

  std::cout << "num1:" << num1 << " num2:" << num2 << std::endl;
  grpc::Status put_status = paxos_1.Start(1, num1);
  put_status = paxos_2.Start(2, num2);
  put_status = paxos_2.Start(2, std::to_string(random()));

  auto [ decided_0, val_0 ] = paxos_0.Status(2);
  auto [ decided_1, val_1 ] = paxos_1.Status(2);
  auto [ decided_2, val_2 ] = paxos_2.Status(2);

  assert(decided_0 == decided_1 && val_0 == val_1 && "s0 and s1 agree");
  assert(decided_1 == decided_2 && val_1 == val_2 && "s1 and s2 agree");

  shutdown_service(paxos_server_0.get());
  shutdown_service(paxos_server_1.get());
  shutdown_service(paxos_server_2.get());

  paxos_thread_0.join();
  paxos_thread_1.join();
  paxos_thread_2.join();

  std::cout << "----------------Test_basic_put: Passed" << std::endl;
  return;
}


int main(int argc, char** argv) {

  test_basic_put();

  int peers_num = 3;
  int put_size = 100;
  std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };
  // std::vector<std::shared_ptr<grpc::Channel>> channels = make_channels(peers_num, addr_v);
  // test_heavy_put(peers_num, addr_v, channels, put_size);

  PaxosServiceImpl paxos_0(peers_num, addr_v, 0);
  paxos_0.InitializeService();
  paxos_0.StartService();
  paxos_0.Start(1, "one");
  paxos_0.Start(2, "two");


}
