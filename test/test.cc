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

  for (int i = 0; i < peers_num; ++i) {
    std::cout << i << " start" << std::endl;
    std::unique_ptr<PaxosServiceImpl> paxos = std::unique_ptr<PaxosServiceImpl>(
      new PaxosServiceImpl(peers_num,addr_v, i)
    );
    paxos->InitializeService();
    paxos->StartService();
    pxs.push_back(std::move(paxos));
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

  std::tuple<bool, std::string> status0;
  std::tuple<bool, std::string> status1;
  std::tuple<bool, std::string> status2;

  // check all saved values
  for (int i = 0; i < put_size; i++){
    int ndecided = 0;
    while(ndecided != peers_num){
      ndecided = 0;
      for (int j = 0; j < peers_num; j++){
        auto [ decided, _ ] = pxs.at(j)->Status(i);
        if (decided == true){
          ndecided += 1;
        }
      }
    }
    auto [ decided_0, val_0 ] = pxs.at(0)->Status(i);
    for (int j = 1; j < peers_num; j++){
        auto [ decided_1, val_1 ] = pxs.at(j)->Status(i);
        assert( val_0 == val_1 );
    }
  }

  // shut down
  for (int i = 0; i < peers_num; i++) {
    pxs.at(i)->TerminateService();
  }

  std::cout << "----------------Test_heavy_put: Passed" << std::endl;
}


void test_basic_put(int peers_num, const std::vector<std::string>& addr_v, std::vector<std::shared_ptr<grpc::Channel>> channels){
   // random number seed
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> distribution(0, 500000);
  auto random = std::bind(distribution, generator);

  // parameters
  //int peers_num = 3;

  //std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };
  //std::vector<std::shared_ptr<grpc::Channel>> channels = make_channels(peers_num, addr_v);
  // ----------------------  manutal creation of the paxos service----------------------

  PaxosServiceImpl paxos_0(peers_num, addr_v, 0);
  PaxosServiceImpl paxos_1(peers_num, addr_v, 1);
  PaxosServiceImpl paxos_2(peers_num, addr_v, 2);
  paxos_0.InitializeService();
  paxos_0.StartService();
  paxos_1.InitializeService();
  paxos_1.StartService();
  paxos_2.InitializeService();
  paxos_2.StartService();

  grpc::Status put_status; 
  put_status = paxos_1.Start(1, std::to_string(random()));
  put_status = paxos_2.Start(2, std::to_string(random()));

  std::tuple<bool, std::string> status0;
  std::tuple<bool, std::string> status1;
  std::tuple<bool, std::string> status2;

  while (true){
    status0 = paxos_0.Status(2);
    status1 = paxos_1.Status(2);
    status2 = paxos_2.Status(2);
  
    if ( std::get<0>(status0) && std::get<0>(status1) && std::get<0>(status2) ){
      break;
    }
  }

  assert(std::get<1>(status0) == std::get<1>(status1) 
    && std::get<1>(status1) == std::get<1>(status2) 
    && "all three servers agree");

  paxos_0.TerminateService();
  paxos_1.TerminateService();
  paxos_2.TerminateService();

  std::cout << "----------------Test_basic_put: Passed" << std::endl;
  return;
}


int main(int argc, char** argv) {

  int peers_num = 3;
  int put_size = 100;
  std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };
  std::vector<std::shared_ptr<grpc::Channel>> channels = make_channels(peers_num, addr_v);
  test_basic_put(peers_num, addr_v, channels);
  test_heavy_put(peers_num, addr_v, channels, put_size);
  return 0;

}
