#include <cstdlib>

#include "paxos.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using paxos::Paxos;
using paxos::Proposal;
using paxos::Response;
using paxos::MetaData;
using paxos::EmptyMessage;


PaxosServiceImpl::PaxosServiceImpl(std::vector<std::string> peers, int me)
    : peers(peers), me(me) {}


grpc::Status PaxosServiceImpl::SimpleReceive(ServerContext* context, const Proposal* proposal, Response* response)
{
  int n = proposal->proposed_num();
  int seq = proposal->seq();
  std::string value = proposal->value();
  MetaData meta = proposal->meta();
  std::string type = proposal->type();

  std::cout << "Server " << peers[me] << " received: type = " << type
            << ", n = " << n << ", seq = " << seq << ", val = " << value
            << ", meta = " << meta.me << " " << meta.done
            << std::endl;

  *Instance ins = instances[seq];
  ins->vd = value;
  MetaData my_meta;
  my_meta->set_me(me);
  my_meta->set_done(0);

  response->set_approved(true);
  response->set_number(seq);
  response->set_value(value);
  response->set_meta(my_meta);
  response->set_value(value);

  return grpc::Status::OK;
}


grpc::Status PaxosServiceImpl::Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response)
{
  return grpc::Status::OK;
}
