/**
 *  @file   test.cc
 *  @brief  Test for Paxos implementation
 *  This file contains some performance tests for checking Paxos implementation.
 *
 *  @author Luofei Zhang
 *  @author Xijiao Li
 *  @author Jiawei Zhang
 *  @date   2021-04-12
 ***********************************************/

#include <chrono>
#include <memory>
#include <string>
#include <thread>
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
using paxos::Paxos;

/// Helper function, wait all paxos decided on a seq number and then check the agreed value.
void wait_check(int start, int end, const std::vector<std::unique_ptr<Paxos>>& pxs, int wanted){
  int peers_num = pxs.size();

  for (int i = start; i <= end; i++){
    int ndecided = 0;
    while(ndecided < wanted){
      ndecided = 0;
      for (int j = 0; j < peers_num; j++){
        auto [ decided, _ ] = pxs.at(j)->Status(i);
        if (decided == true){
          ndecided += 1;
        }
      }
    }
    auto [ decided_0, val_0 ] = pxs.at(0)->Status(i);
    int nagreed = 1;
    for (int j = 1; j < peers_num; j++){
        auto [ decided_1, val_1 ] = pxs.at(j)->Status(i);
        if ( val_0 == val_1 ){
          nagreed++;
        }
    }
    assert( nagreed >= wanted);
  }
  return;
}

/// Helper function, wait all paxos to decided on a seq number while some of them might have been dead
void wait_fail(int start, int end, const std::vector<std::unique_ptr<Paxos>>& pxs, int wanted){
  int peers_num = pxs.size();

  for (int i = start; i <= end; i++){
    int ndecided = 0;
    int round = 0;
    while(ndecided < wanted && round < 10){
      ndecided = 0;
      round++;
      for (int j = 0; j < peers_num; j++){
        auto [ decided, _ ] = pxs.at(j)->Status(i);
        if (decided == true){
          ndecided += 1;
        }
      }
    }
    assert(ndecided == 0);
  }
  return;
}


/// Test paxos's performance undering heavy put
void test_heavy_put(const std::vector<std::string>& addr_v, int put_size)
{
  int peers_num = addr_v.size();
  std::vector<std::unique_ptr<Paxos>> pxs;

  for (int i = 0; i < peers_num; ++i) {
    std::cout << i << " start" << std::endl;
    std::unique_ptr<Paxos> paxos = std::unique_ptr<Paxos>(
      new Paxos(addr_v, i)
    );
    paxos->InitializeService();
    paxos->StartService();
    pxs.push_back(std::move(paxos));
  }

  // random number seed
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> distribution(0, 500000);
  auto random = std::bind(distribution, generator);

  auto before = std::chrono::system_clock::now();
  // heavy put
  for (int i = 0; i < put_size; i++){
    int server_num = random()%peers_num;
    grpc::Status put_status = pxs.at(server_num)->Start(i, "hi");
  }
  auto after = std::chrono::system_clock::now();
  auto duration = after - before;
  std::cout << "Heavy put takes " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
            << " milliseconds\n";
  wait_check(0, put_size-1, pxs, peers_num);

  // shut down
  for (int i = 0; i < peers_num; i++) {
    pxs.at(i)->TerminateService();
  }

  std::cout << "----------------Test_heavy_put: Passed" << std::endl;
}


/// Test paxos's performance undering basic put
void test_basic_put(const std::vector<std::string>& addr_v){
   // random number seed
  int peers_num = addr_v.size();
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> distribution(0, 500000);
  auto random = std::bind(distribution, generator);

  // init paxos peers
  Paxos paxos_0(addr_v, 0);
  Paxos paxos_1(addr_v, 1);
  Paxos paxos_2(addr_v, 2);
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


/// Test paxos's performance when some of the peers have unreliable connection
void test_unreliable(const std::vector<std::string>& addr_v) {

  std::vector<std::unique_ptr<Paxos>> pxs;
  int peers_num = addr_v.size();

  for (int i = 0; i < peers_num; ++i) {
    std::unique_ptr<Paxos> paxos = std::unique_ptr<Paxos>(
      new Paxos(addr_v, i)
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

  pxs.at(1)->Start(0, std::to_string(random()));
  pxs.at(0)->Start(1, std::to_string(random()));
  pxs.at(2)->Start(2, std::to_string(random()));
  pxs.at(1)->Start(3, std::to_string(random()));
  pxs.at(3)->Start(4, std::to_string(random()));

  // check all saved values
  wait_check(0,4,pxs,4);
  std::cout << "pass test befor a few server die" << std::endl;

  // make a minority of server die
  std::set<int> deadServer = {3,4};
  for (auto i : deadServer) {
    pxs.at(i)->TerminateService();
  }
  std::cout << "server 3 and server 4 is down" << std::endl;

  pxs.at(0)->Start(5,std::to_string(random()));
  pxs.at(1)->Start(6,std::to_string(random()));

  wait_check(5,6,pxs,3);

  std::cout << "alive servers function as expected" << std::endl;
  // shut down alive the service
  for (int i=0; i<peers_num; i++) {
    if (deadServer.count(i)==0)
      pxs.at(i)->TerminateService();
  }
  std::cout << "----------------Test_unreliable: Passed" << std::endl;
  return;
}


/// Test paxos's performance when some peers are dead and only less then a half are alive
void test_minority(const std::vector<std::string>& addr_v) {
  int peers_num = addr_v.size();
  std::vector<std::unique_ptr<Paxos>> pxs;

  for (int i = 0; i < peers_num; ++i) {
    std::unique_ptr<Paxos> paxos = std::unique_ptr<Paxos>(
      new Paxos(addr_v, i)
    );
    paxos->InitializeService();
    paxos->StartService();
    pxs.push_back(std::move(paxos));
    std::cout << i << " ok" << std::endl;
  }

  // make a majority of server die
  std::set<int> deadServer = {2,3,4};
  for (auto i : deadServer) {
    pxs.at(i)->TerminateService();
  }
  std::cout << "server 2 and server 3 and server 4 is down" << std::endl;

  pxs.at(0)->Start(0,"0");
  pxs.at(1)->Start(1,"1");
  wait_fail(0, 1, pxs, peers_num);

  std::cout << "minority servers cannot make any progress" << std::endl;
  // shut down alive the service
  for (int i=0; i<peers_num; i++) {
    if (deadServer.count(i)==0)
      pxs.at(i)->TerminateService();
  }
  std::cout << "----------------Test_minority: Passed" << std::endl;
  return;
}


/// Test paxos's performance when there are concurrent propose to send proposal for one same seq number
void test_concurrent(const std::vector<std::string>& addr_v) {

  int peers_num = addr_v.size();
  std::vector<std::unique_ptr<Paxos>> pxs;

  for (int i = 0; i < peers_num; ++i) {
    std::unique_ptr<Paxos> paxos = std::unique_ptr<Paxos>(
      new Paxos(addr_v, i)
    );
    paxos->InitializeService();
    paxos->StartService();
    pxs.push_back(std::move(paxos));
    std::cout << i << " ok" << std::endl;
  }

  pxs.at(1)->Start(1,"hi");
  pxs.at(2)->Start(1, "2");
  pxs.at(0)->Start(1,"hello");

  pxs.at(1)->Start(2,"hi2");
  pxs.at(2)->Start(2, "22");
  pxs.at(0)->Start(2,"hello2");

  pxs.at(1)->Start(1,"1");

  wait_check(1,2, pxs, peers_num);

  // shut down alive the service
  for (int i=0; i< peers_num; i++) {
    pxs.at(i)->TerminateService();
  }
  std::cout << "----------------Test_concurrent: Passed" << std::endl;
  return;
}


/// Main entry for running paxos tests
int main(int argc, char** argv) {

  std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };
  test_basic_put(addr_v);

  int put_size = 1000;
  std::vector<std::string> addr_v_heavy {"0.0.0.0:50061", "0.0.0.0:50062", "0.0.0.0:50063" };
  std::cout << put_size << std::endl;
  test_heavy_put(addr_v_heavy, put_size);

  std::vector<std::string> addr_v_unreliable {"0.0.0.0:50071", "0.0.0.0:50072", "0.0.0.0:50073",  "0.0.0.0:50074", "0.0.0.0:50075"};
  test_unreliable(addr_v_unreliable);

  std::vector<std::string> addr_v_minority {"0.0.0.0:50081", "0.0.0.0:50082", "0.0.0.0:50083",  "0.0.0.0:50084", "0.0.0.0:50085"};
  test_minority(addr_v_minority);

  std::vector<std::string> addr_v_concurent {"0.0.0.0:50091", "0.0.0.0:50092", "0.0.0.0:50093",  "0.0.0.0:50094", "0.0.0.0:50095"};
  test_concurrent(addr_v_concurent);

  return 0;

}
