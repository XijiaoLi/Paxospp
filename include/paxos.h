/**
 *  @file   paxos.h
 *  @brief  Paxos Service Implementation Interface
 *  This file contains the prototypes for the Paxos Service
 *  and some strucures that you will need.
 *
 *  @author Xijiao Li
 *  @date   2021-04-12
 ***********************************************/

#ifndef PAXOS_H
#define PAXOS_H

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <tuple>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <future>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "paxos.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using paxos::PaxosService;
using paxos::Proposal;
using paxos::Response;
using paxos::EmptyMessage;

namespace paxos {

/**
 * \struct Proposer
 * \brief A structure to keep the states when paxos act as a Proposer
 */
typedef struct Proposer {
  int n;
  int np;
} Proposer;

/**
 * \struct Acceptor
 * \brief A structure to keep the states when paxos act as an Acceptor
 */
typedef struct Acceptor {
  int np;
  int na;
  std::string va;
} Acceptor;

/**
 * \struct Instance
 * \brief A structure to represent an Instance
 */
typedef struct Instance {
  std::shared_mutex mu;
  Proposer p;
  Acceptor a;
  std::string vd;
} Instance;


/**
 * \class Paxos
 * \brief A class for the implementation for Paxos, to be included in an application.
 *
 * This file defines the suggested RPC protocol between instances of Paxos running on different nodes,
 * including structures for arguments and return types. The procedures correspond roughly to the Paxos pseudocode in Tutorial.
 * Manages a sequence of agreed-on values. The set of peers is fixed. Copes with network failures (partition, msg loss, &c).
 */
class Paxos final : public PaxosService::Service {
  public:
    /// Constructor
    Paxos(std::vector<std::string> peers_addr, int me);

    /// Constructor with debug flag
    Paxos(std::vector<std::string> peers_addr, int me, bool debug);

    /// Initialize server, channel, stub
    void InitializeService();

    /// Server starts to listen on the address
    void StartService();

    /// Shut down the service on the server
    void TerminateService();

    /// Ping service for checking aliveness
    grpc::Status Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response) override;

    /// Receive service for communication between paxos peers
    grpc::Status Receive(ServerContext* context, const Proposal* proposal, Response* response) override;

    /// Main entry point for running paxos Receive service
    grpc::Status Start(int seq, std::string v);

    /// Check a paxos peer's decision on an instance
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
    std::vector<std::unique_ptr<PaxosService::Stub>> peers;
    std::vector<std::shared_ptr<grpc::Channel>> channels;
    bool initialized;
    mutable std::shared_mutex mu;
    mutable std::shared_mutex acceptor_lock;
    bool dead;
    std::map<int, Instance*> instances;
    std::unique_ptr<std::thread> listener;
    std::vector<std::future<bool>> request_threads;
}; // end of class Paxos

} // end of namespace paxos

#endif
