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

// void show_usage(char* name)
// {
//   std::cerr << "Usage: " << name << " <option(s)> \n"
//             << "Options:\n"
//             << "\t-h\tShow this help message\n"
//             << "\t-n\t<int>\tSpecify the index of kv server among its peers\n"
//             << "\t-p\t<address> <address>...\tSpecify the list of address of paxos servers\n"
//             << "\t-s\t<address>\tSpecify the address of kv server\n"
//             << std::endl;
// }
//
// bool legal_int(char *str) {
//   while (*str) {
//     if (!isdigit(*str++)) {
//       return false;
//     }
//   }
//   return true;
// }

int main(int argc, char** argv) {

  // std::cerr << argc << "\n";
  // if (argc < 1) {
  //
  //   return -EINVAL;
  // }
  //
  // std::vector<std::string> paxos_addr;
  // std::string server_addr;
  // int me, peers_num;
  //
  // int arg_i = 0;
  // while (arg_i < argc) {
  //   arg_i ++;
  //   if (strcmp(argv[arg_i], "-p") == 0) {
  //     arg_i ++;
  //     while (arg_i < argc && argv[arg_i][0] != '-') {
  //       paxos_addr.push_back(argv[arg_i]);
  //       arg_i++;
  //     }
  //     peers_num = paxos_addr.size();
  //     if (peers_num == 0) {
  //       show_usage(argv[0]);
  //       return 0;
  //     }
  //   } else if (strcmp(argv[arg_i], "-h") == 0) {
  //     show_usage(argv[0]);
  //     return 0;
  //   } else if (strcmp(argv[arg_i], "-s") == 0) {
  //     arg_i ++;
  //     if (arg_i < argc && argv[arg_i][0] != '-') {
  //       server_addr = argv[arg_i];
  //     } else {
  //       show_usage(argv[0]);
  //       return 0;
  //     }
  //   } else if (strcmp(argv[arg_i], "-n") == 0) {
  //     arg_i ++;
  //     if (arg_i < argc && legal_int(argv[arg_i])) {
  //       me = atoi(argv[arg_i]);
  //     } else {
  //       show_usage(argv[0]);
  //       return 0;
  //     }
  //   }
  // }

  std::vector<std::string> addr_v {"0.0.0.0:50051", "0.0.0.0:50052"};

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> distribution(0, 500000);
  auto random = std::bind(distribution, generator);

  PaxosServiceImpl paxos(addr_v, 0, true);
  paxos.InitializeService();
  paxos.StartService();

  PaxosServiceImpl peer_1(addr_v, 1, true);
  peer_1.InitializeService();
  peer_1.StartService();

  grpc::Status put_status;
  put_status = paxos.Start(1, std::to_string(random()));

  for (;;){}

  return 0;

}
