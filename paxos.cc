#include <cstdlib>

#include "paxos.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;

using paxos::Paxos;
using paxos::Proposal;
using paxos::Response;
using paxos::EmptyMessage;


std::vector<std::unique_ptr<Paxos::Stub>> make_stubs(int replica_size, std::vector<std::shared_ptr<grpc::Channel>> channels)
{
  std::vector<std::unique_ptr<Paxos::Stub>> peers; // a list of stubs

  for (int i = 0; i < replica_size; ++i) {
    // create a stub associated with the channel
    std::unique_ptr<Paxos::Stub> peer_i = std::make_unique<Paxos::Stub>(channels[i]);
    peers.push_back(std::move(peer_i));
    std::cout << "Adding peer " << i << " to the Paxos stubs list ..." << std::endl;
  }

  return peers;
}


PaxosServiceImpl::PaxosServiceImpl(int replica_size, std::vector<std::shared_ptr<grpc::Channel>> channels, int me)
  : peers(std::move(make_stubs(replica_size, channels))), me(me) {}


grpc::Status PaxosServiceImpl::SimpleReceive(ServerContext* context, const Proposal* proposal, Response* response)
{
  std::string type = proposal->type();
  int n = proposal->proposed_num();
  int seq = proposal->seq();
  std::string value = proposal->value();
  int peer = proposal->me();
  int peer_done = proposal->done();

  std::cout << "Server " << me << " received from Client " << peer
            << "\n\t type = " << type << ", n = " << n
            << ", seq = " << seq << ", val = " << value
            << std::endl;

  Instance ins = instances[seq];
  std::cout << "\t initial value is " << ins.vd << ", new val is " << value << std::endl;
  ins.vd = value;
  instances[seq] = ins;

  response->set_type("server message");
  response->set_approved(true);
  response->set_number(n);
  response->set_value(value);
  response->set_me(me);
  response->set_done(0);

  return grpc::Status::OK;
}


grpc::Status PaxosServiceImpl::Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response)
{
  return grpc::Status::OK;
}


grpc::Status PaxosServiceImpl::Run(int seq, std::string v)
{
  int count = 0;
  for (const auto& stub : peers) {
    ClientContext context;

    Proposal proposal;
    proposal.set_type("client message");
    proposal.set_proposed_num(count);
    proposal.set_seq(seq);
    proposal.set_value(v);
    proposal.set_me(me);
    proposal.set_done(0);

    Response response;

    std::cout << "Client " << me << " sent to Peer " << count << std::endl;

    grpc::Status status = stub->SimpleReceive(&context, proposal, &response);

    if (!status.ok()) {
      std::cout << "Client " << me << " received from Peer " << count << " FAILED!" << std::endl;
    } else {
      std::string type = response.type();
      bool approved = response.approved();
      int n = response.number();
      std::string value = response.value();
      int peer = response.me();
      int peer_done = response.done();

      std::cout << "Client " << me << " received from Peer " << count << " with me = " << peer
                << "\n\t type = " << type << ", n = " << n
                << ", seq = " << seq << ", val = " << value
                << std::endl;
    }

    count ++;

  }
  return grpc::Status::OK;
}
