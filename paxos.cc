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

// ---------------------------- Helper Function ----------------------------

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


// ----------------------- PaxosServiceImpl Function -----------------------


PaxosServiceImpl::PaxosServiceImpl(int replica_size, std::vector<std::shared_ptr<grpc::Channel>> channels, int me)
  : peers(std::move(make_stubs(replica_size, channels))), me(me) {}

/* TODO: implement later */
int PaxosServiceImpl::Min()
{
  return 0;
}


grpc::Status PaxosServiceImpl::Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response)
{
  return grpc::Status::OK;
}


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

  std::map<int, Instance*>::iterator it;

  it = instances.find(seq);
  if (it != instances.end()) {
    Instance* ins_prt = it->second;
    std::cout << "\t initial value is " << ins_prt->vd << ", new val is " << value << std::endl;
    ins_prt->vd = value;
  } else {
    Instance* ins_prt;
    ins_prt = (Instance*)malloc( sizeof( Instance ) );
    ins_prt->vd = value;
    std::cout << "\t initial value is null, new val is " << value << std::endl;
    instances[seq] = ins_prt;
  }

  response->set_type("server message");
  response->set_approved(true);
  response->set_number(n);
  response->set_value(value);
  response->set_me(me);
  response->set_done(0);

  return grpc::Status::OK;
}


grpc::Status PaxosServiceImpl::Receive(ServerContext* context, const Proposal* proposal, Response* response)
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


// Instance* PaxosServiceImpl::get_instance(int seq)
// {
//
// }

//
// grpc::Status PaxosServiceImpl::Start(int seq, std::string v)
// {
//   if (seq < Min()) {
// 		return grpc::Status(grpc::StatusCode::ABORTED, "Aborted: seq num is too low.");
// 	}
// 	Instance instance = px.getInstance(seq)
// 	instance.mu.Lock()
// 	defer instance.mu.Unlock()
// 	for !px.dead {
// 		if instance.decidedValue != nil {
// 			break
// 		}
// 		instance.proposer.highestSeenProposedNumber++
// 		instance.proposer.proposedNumber = instance.proposer.highestSeenProposedNumber
// 		ok, value := px.propose(instance, seq)
// 		if !ok {
// 			continue
// 		}
// 		if value != nil {
// 			v = value
// 		}
// 		if !px.requestAccept(instance, seq, v) {
// 			continue
// 		}
// 		px.decide(seq, v)
// 		break
// 	}
//
//
//
//   int count = 0;
//   for (const auto& stub : peers) {
//     ClientContext context;
//
//     Proposal proposal;
//     proposal.set_type("client message");
//     proposal.set_proposed_num(count);
//     proposal.set_seq(seq);
//     proposal.set_value(v);
//     proposal.set_me(me);
//     proposal.set_done(0);
//
//     Response response;
//
//     std::cout << "Client " << me << " sent to Peer " << count << std::endl;
//
//     grpc::Status status = stub->SimpleReceive(&context, proposal, &response);
//
//     if (!status.ok()) {
//       std::cout << "Client " << me << " received from Peer " << count << " FAILED!" << std::endl;
//     } else {
//       std::string type = response.type();
//       bool approved = response.approved();
//       int n = response.number();
//       std::string value = response.value();
//       int peer = response.me();
//       int peer_done = response.done();
//
//       std::cout << "Client " << me << " received from Peer " << count << " with me = " << peer
//                 << "\n\t type = " << type << ", n = " << n
//                 << ", seq = " << seq << ", val = " << value
//                 << std::endl;
//     }
//
//     count ++;
//
//   }
//   return grpc::Status::OK;
// }
