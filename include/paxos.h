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
  int n; // proposedNumber
  int np; // highestSeenProposedNumber
};

struct Acceptor {
  int np; // highestProposedNumber
  int na; // highestAcceptedNumber
  std::string va; // highestAcceptedValue
};

struct Instance {
  std::shared_mutex mu; // mu
  Proposer p; // proposer
  Acceptor a; // acceptor
  std::string vd; // decidedValue
};

class PaxosServiceImpl final : public Paxos::Service {
  
  public:
    PaxosServiceImpl(int peers_num, std::vector<std::string> peers_addr, int me);
    PaxosServiceImpl(int peers_num, std::vector<std::string> peers_addr, int me, bool debug);

    // Initialize server, channel, stub
    void InitializeService();
    // Start listening on the address
    void StartService();
    // Shut down the service on the server
    void TerminateService();

    // Paxos Ping service test if the server is available
    grpc::Status Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response) override;
    // Paxos Receice service to receive proposals
    grpc::Status Receive(ServerContext* context, const Proposal* proposal, Response* response) override;
    
    // Main entry point for running Paxos service
    grpc::Status Start(int seq, std::string v);
    // Check a paxos peer's decision on an instance
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
    std::unique_ptr<grpc::Server> server;
    std::vector<std::unique_ptr<Paxos::Stub>> peers;
    std::vector<std::shared_ptr<grpc::Channel>> channels;
    mutable std::shared_mutex mu;
    mutable std::shared_mutex acceptor_lock;
    std::map<int, Instance*> instances;
    std::unique_ptr<std::thread> listener;
    std::vector<std::future<bool>> request_threads;
    bool dead;
  

};

#endif
