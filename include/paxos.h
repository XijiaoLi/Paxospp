#ifndef PAXOS_H
#define PAXOS_H

#include <iostream>
#include <memory>
#include <string> // std::string
#include <map> // std::map
#include <tuple> // std::tuple
#include <thread> // std::thread
#include <mutex>  // std::unique_lock
#include <shared_mutex> //std::shared_mutex
#include <future> // std::future

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "paxos.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using paxos::Paxos;
using paxos::Proposal;
using paxos::Response;
using paxos::EmptyMessage;


struct Proposer {
  int n;
  int np;
};

struct Acceptor {
  int np;
  int na;
  std::string va;
};

struct Instance {
  std::shared_mutex mu;
  Proposer p;
  Acceptor a;
  std::string vd;
};

class PaxosServiceImpl final : public Paxos::Service {
  public:
    PaxosServiceImpl(std::vector<std::string> peers_addr, int me);
    PaxosServiceImpl(std::vector<std::string> peers_addr, int me, bool debug);

    // initialize server, channel, stub
    void InitializeService();

    // start listening on the address
    void StartService();

    // shut down the service on the server
    void TerminateService();

    // test if the server is available
    grpc::Status Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response) override;

    // paxos service
    grpc::Status Receive(ServerContext* context, const Proposal* proposal, Response* response) override;

    // main entry point for running paxos Receive service
    grpc::Status Start(int seq, std::string v);

    // check a paxos peer's decision on an instance
    std::tuple<bool, std::string> Status(int seq);

  private:
    void start_service();
    bool start(int seq, std::string v);
    Instance* get_instance(int seq);
    std::tuple<bool, std::string> propose(Instance* instance, int seq);
    bool request_accept(Instance* instance, int seq, std::string v);
    void decide(int seq, std::string v);

    int peers_num;
    int me;
    bool debug;
    std::unique_ptr<grpc::Server> server;
    std::vector<std::string> peers_addr;
    std::vector<std::unique_ptr<Paxos::Stub>> peers;
    std::vector<std::shared_ptr<grpc::Channel>> channels;
    bool initialized;
    mutable std::shared_mutex mu;
    mutable std::shared_mutex acceptor_lock;
    bool dead;
    std::map<int, Instance*> instances;
    std::unique_ptr<std::thread> listener;
    std::vector<std::future<bool>> request_threads;
};

#endif
