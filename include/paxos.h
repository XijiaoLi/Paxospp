#ifndef PAXOS_H
#define PAXOS_H
#endif

#include <iostream>
#include <memory>
#include <string> // std::string
#include <map> // std::map
#include <tuple> // std::tuple
#include <mutex>  // std::unique_lock
#include <shared_mutex> //std::shared_mutex
// #include <any> // std::any

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

// typedef std::map<int, *Instance> InstanceMap;

class PaxosServiceImpl final : public Paxos::Service {
  public:

    PaxosServiceImpl(int replica_size, std::vector<std::shared_ptr<grpc::Channel>> channels, int me);

    // test if the server is available
    grpc::Status Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response) override;

    // store anything in the proposal
    grpc::Status SimpleReceive(ServerContext* context, const Proposal* proposal, Response* response) override;

    // paxos service
    grpc::Status Receive(ServerContext* context, const Proposal* proposal, Response* response) override;

    // main entry point for running SimpleReceive service
    grpc::Status SimpleStart(int seq, std::string v);

    // main entry point for running paxos Receive service
    grpc::Status Start(int seq, std::string v);

    int Min();

  private:

    // MetaData init_meta();
    // void update_meta(MetaData meta);
    // void clean_done_values();

    Instance* get_instance(int seq);
    std::tuple<bool, std::string> propose(Instance* instance, int seq);
    bool request_accept(Instance* instance, int seq, std::string v);
    void decide(int seq, std::string v);

    // below are list of fields in PaxosServiceImpl class,
    // corresponding to line 34-39 in paxos.go

    /* TODO: will implement in later version
    bool                     unreliable; // unreliable
    int                      rpc_count; // rpcCount
    int                      max_seq; // maxSeq
    int[]                    done_map; // doneMap
    int                      to_clean_seq; // toCleanSeq
    */

    std::vector<std::unique_ptr<Paxos::Stub>> peers; // peers
    int me; // me
    mutable std::shared_mutex mu; // mu
    mutable std::shared_mutex acceptor_lock; // acceptorLock
    bool dead; // dead
    std::map<int, Instance*> instances; // instances (seq -> instance)

};
