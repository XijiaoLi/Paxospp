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
using paxos::EProposal;
using paxos::EResponse;
using paxos::Proposal;
using paxos::Response;
using paxos::MetaData;


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
	std::shared_mutex mu;
	Proposer p; // proposer
	Acceptor a; // acceptor
	std::any vd; // decidedValue
};

typedef std::map<int, *Instance> InstanceMap;

class PaxosServiceImpl final : public Paxos::Service {
  public:
    grpc::Status Echo(ServerContext* context, const EProposal* eproposal, EResponse* eresponse) override;

    grpc::Status Receive(ServerContext* context, const Proposal* proposal, Response* response) override;

    // paxos phase 1
    // grpc::Status Prepare(ServerContext* context, const Proposal* proposal, Response* response) override;

    // paxos phase 2
    // grpc::Status Accept(ServerContext* context, const Proposal* proposal, Response* response) override;

    // paxos phase 3
    // grpc::Status Decide(ServerContext* context, const Proposal* proposal, Response* response) override;

    // test if the server is available
    grpc::Status Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response) override;

    void start();

    // after brought up again, a server will catch up with others' logs.
    // grpc::Status Recover(grpc::ServerContext* context, const EmptyMessage* request, RecoverResponse* response) override;
    // grpc::Status Initialize();

  private:

    *Instance get_instance(int seq);
    std::tuple<bool, std::any> propose(*Instance instance, int seq);
    bool request_accept(*Instance instance, int seq, std::any value);
    void decide(int seq, std::any value);
    MetaData init_meta();
    void update_meta(MetaData meta);
    void clean_done_values();

    // template <typename Request>
    // grpc::Status RunPaxos(const Request& req);
    // grpc::Status GetCoordinator();
    // grpc::Status ElectNewCoordinator();
    // grpc::Status GetRecovery();
    // bool RandomFail();

    std::shared_mutex mu;
    bool              dead;
  	bool              unreliable;
    int               rpc_count;
    std::string       peers[];
    const int         me;

    std::shared_mutex acceptor_mu; //acceptorLock sync.Mutex
	  InstanceMap       instances;
	  int               max_seq;
	  int[]             done_map;
	  int               to_clean_seq;

};
