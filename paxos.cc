#include <cstdlib>

#include "paxos.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;

using paxos::Paxos;
using paxos::Proposal;
using paxos::Response;
using paxos::MetaData;
using paxos::EmptyMessage;


PaxosServiceImpl::PaxosServiceImpl(std::vector<std::unique_ptr<Paxos::Stub>>* peers, int me)
    : peers(peers), me(me) {}


grpc::Status PaxosServiceImpl::SimpleReceive(ServerContext* context, const Proposal* proposal, Response* response)
{
  int n = proposal->proposed_num();
  int seq = proposal->seq();
  std::string value = proposal->value();
  MetaData meta = proposal->meta();
  std::string type = proposal->type();

  std::cout << "Server " << me << " received from Client " << meta.me()
            << ": type = " << type << ", n = " << n
            << ", seq = " << seq << ", val = " << value
            << std::endl;

  // Instance* ins = instances[seq];
  // ins->vd = value;
  MetaData my_meta;
  my_meta.set_me(me);
  my_meta.set_done(0);

  response->set_approved(true);
  response->set_number(seq);
  response->set_value(value);
  response->set_allocated_meta(&my_meta);
  response->set_type("server message");

  return grpc::Status::OK;
}


grpc::Status PaxosServiceImpl::Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response)
{
  return grpc::Status::OK;
}


grpc::Status PaxosServiceImpl::Run(int seq, std::string v)
{
  int count = 0;
  for (const auto& stub : *peers) {
    count ++;
    ClientContext context;
    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(5000);
    context.set_deadline(deadline);

    MetaData meta;
    meta.set_me(me);
    meta.set_done(0);

    Proposal proposal;
    proposal.set_proposed_num(count);
    proposal.set_seq(seq);
    proposal.set_value(v);
    proposal.set_allocated_meta(&meta);
    proposal.set_type("client message");

    Response response;

    grpc::Status status = stub->SimpleReceive(&context, proposal, &response);
    if (!status.ok()) {
      std::cout << "Client " << me << " received from Peer " << count << " failed" << std::endl;
    } else {
      bool approved = response.approved();
      int n = response.number();
      std::string value = response.value();
      MetaData meta = response.meta();
      std::string type = response.type();
      std::cout << "Client " << me << " received from Peer " << count << " with meta.me = " << meta.me()
                << ": type = " << type << ", n = " << n
                << ", seq = " << seq << ", val = " << value
                << std::endl;
    }
  }
  return grpc::Status::OK;
}
