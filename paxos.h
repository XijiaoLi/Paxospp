#ifndef PAXOS_H
#define PAXOS_H
#endif

#include <iostream>
#include <memory>
#include <string> // std::string
#include <any> // std::any
#include <map> // std::map
#include <tuple> // std::tuple

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
using paxos::MetaData;
using paxos::EmptyMessage;


struct Proposer {
  int n; // proposedNumber
  int np; // highestSeenProposedNumber
};

struct Acceptor {
  int np; // highestProposedNumber
  int na; // highestAcceptedNumber
  std::any va; // highestAcceptedValue
};

struct Instance {
  std::shared_mutex mu; // mu
  Proposer p; // proposer
  Acceptor a; // acceptor
  std::any vd; // decidedValue
};

// typedef std::map<int, *Instance> InstanceMap;

class PaxosServiceImpl final : public Paxos::Service {
  public:

    PaxosServiceImpl(std::vector<std::string> peers, int me);

    // test if the server is available
    grpc::Status Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response) override;

    // store anything in the proposal
    grpc::Status SimpleReceive(ServerContext* context, const Proposal* proposal, Response* response) override;

    /* TODO: will implement in later version
    grpc::Status Receive(ServerContext* context, const Proposal* proposal, Response* response) override;
    */

    void start();

  private:

    *Instance get_instance(int seq);
    std::tuple<bool, std::any> propose(*Instance instance, int seq);
    bool request_accept(*Instance instance, int seq, std::any value);
    void decide(int seq, std::any value);
    MetaData init_meta();
    void update_meta(MetaData meta);
    void clean_done_values();

    // below are list of fields in PaxosServiceImpl class,
    // corresponding to line 34-39 in paxos.go

    /* TODO: will implement in later version
    std::shared_mutex        mu;
    bool                     dead; // dead
    bool                     unreliable; // unreliable
    int                      rpc_count; // rpcCount
    int                      max_seq; // maxSeq
    int[]                    done_map; // doneMap
    int                      to_clean_seq; // toCleanSeq
    */

    std::vector<std::string> peers; // peers
    const int                me; // me
    std::shared_mutex        acceptor_mu; //acceptorLock
    std::map<int, *Instance> instances; // instances (seq -> instance)

};
