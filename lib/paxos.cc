#include <cstdlib>
#include <algorithm>    // std::max

#include "../include/paxos.h"

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
  : peers(std::move(make_stubs(replica_size, channels))), me(me), dead(false) {}

/* TODO: implement later */
int PaxosServiceImpl::Min()
{
  return 0;
}


/* Ping service for checking aliveness */
grpc::Status PaxosServiceImpl::Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response)
{
  return grpc::Status::OK;
}


/* Receive service for communication between paxos peers */
grpc::Status PaxosServiceImpl::Receive(ServerContext* context, const Proposal* proposal, Response* response)
{
  std::string type = proposal->type();
  int n = proposal->proposed_num();
  int seq = proposal->seq();
  std::string value = proposal->value();
  int peer = proposal->me();
  int peer_done = proposal->done();

  std::unique_lock<std::shared_mutex> lock(acceptor_lock);
  Instance* instance = get_instance(seq);
  response->set_me(me);
  response->set_done(0);

  if (type.compare("PROPOSE") == 0) {
    if (n <= (instance->a).np) {
			response->set_approved(false);
			response->set_number((instance->a).np);
		} else {
			(instance->a).np = n;
			response->set_approved(true);
			response->set_number((instance->a).na);
			response->set_value((instance->a).va);
		}
  } else if (type.compare("ACCEPT") == 0) {
    if (n < instance->a.np) {
      response->set_approved(false);
			response->set_number((instance->a).np);
		} else {
			(instance->a).np = n;
			(instance->a).na = n;
			(instance->a).va = value;
			response->set_approved(true);
			response->set_number(n); // unnecessary?
		}
  } else if (type.compare("DECIDE") == 0) {

    std::cout << "SPaxos " << me << " from CPaxos " << peer
              << "\n\t DECIDE (seq, val) = (" << seq << ", " << value << ")"
              << " from initial value \'" << instance->vd << "\'" << std::endl;

    instance->vd = value;
    response->set_approved(true);
  }
  return grpc::Status::OK;
}


Instance* PaxosServiceImpl::get_instance(int seq)
{
  std::unique_lock<std::shared_mutex> lock(mu);

  std::map<int, Instance*>::iterator it;
  Instance* instance;

  it = instances.find(seq);
  if (it != instances.end()) {
    instance = it->second;
  } else {
    instance = new Instance;
    instances[seq] = instance;
  }

  return instance;
}


std::tuple<bool, std::string> PaxosServiceImpl::propose(Instance* instance, int seq)
{
  int count = 0;
	int highest_np = (instance->p).np;
	int highest_na = -1;
	std::string highest_va;

  int i = 0; // only for logging usage
  for (const auto& stub : peers) {

    ClientContext context;

    // args := &Proposal{PROPOSE, instance.proposer.proposedNumber, seq, nil, px.initMeta()}
    Proposal proposal;
    proposal.set_type("PROPOSE");
    proposal.set_proposed_num((instance->p).n);
    proposal.set_seq(seq);
    proposal.set_value("");
    proposal.set_me(me);
    proposal.set_done(0);

    // reply := Response{}
    Response response;

    std::cout << "CPaxos " << me << " sent PROPOSE to SPaxos " << i << std::endl;

    grpc::Status status = stub->Receive(&context, proposal, &response);
    // if !flag { continue }
    if (!status.ok()) {
			continue;
		}

		// TODO: px.updateMeta(reply.Meta)

    bool approved = response.approved();
    int n = response.number();
    std::string va = response.value();

		if (approved) {
			count++;
			if (n > highest_na) {
				highest_na = n;
				highest_va = va;
			}
		} else {
			highest_np = std::max(highest_np, n);
		}

    i++;
	}

	(instance->p).np = highest_np;
	if (count * 2 > peers.size()) {
		return std::make_tuple(true, highest_va);
	} else {
		return std::make_tuple(false, "");
	}

}


bool PaxosServiceImpl::request_accept(Instance* instance, int seq, std::string v)
{
  int highest_np = (instance->p).np;
	int count = 0;

  int i = 0;
  for (const auto& stub : peers) {

    ClientContext context;

    // args := &Proposal{ACCEPT, instance.proposer.proposedNumber, seq, value, px.initMeta()}
    Proposal proposal;
    proposal.set_type("ACCEPT");
    proposal.set_proposed_num((instance->p).n);
    proposal.set_seq(seq);
    proposal.set_value(v);
    proposal.set_me(me);
    proposal.set_done(0);

    // reply := Response{}
    Response response;

    std::cout << "CPaxos " << me << " sent ACCEPT to SPaxos " << i << std::endl;

    grpc::Status status = stub->Receive(&context, proposal, &response);

    if (status.ok()) {

      // TODO: px.updateMeta(reply.Meta)

      bool approved = response.approved();
      int n = response.number();

  		if (approved) {
  			count++;
  		} else {
  			highest_np = std::max(highest_np, n);
  		}
    }

    i++;
	}

	(instance->p).np = highest_np;
	return count * 2 > peers.size();
}


void PaxosServiceImpl::decide(int seq, std::string v)
{
  std::vector<bool> records (peers.size(), false);
  int count = 0;

  for (; count < peers.size();) {
    int i = 0;

    for (const auto& stub : peers) {
  		if (records[i]) {
  			continue;
  		}

      ClientContext context;

      Proposal proposal;
      proposal.set_type("DECIDE");
      proposal.set_proposed_num(0);
      proposal.set_seq(seq);
      proposal.set_value(v);
      proposal.set_me(me);
      proposal.set_done(0);

      Response response;

      std::cout << "CPaxos " << me << " sent DECIDE to SPaxos " << i << std::endl;

      grpc::Status status = stub->Receive(&context, proposal, &response);

      if (!status.ok()) {
        std::cout << "CPaxos " << me << " got from SPaxos " << i << " FAILED!" << std::endl;
      } else {
        bool approved = response.approved();
        int peer = response.me();
        int peer_done = response.done();

        std::cout << "CPaxos " << me << " got from SPeer " << i << " with me = " << peer
                  << ", approved = " << approved << std::endl;

        if (approved) {
          records[i] = true;
          count ++;
        }
        i ++;
      }
    }
  }
  return;
}


grpc::Status PaxosServiceImpl::Start(int seq, std::string v)
{
  if (seq < Min()) {
		return grpc::Status(grpc::StatusCode::ABORTED, "Aborted: seq num is too low.");
	}

  Instance* instance = get_instance(seq);
  // instance.mu.Lock()
	// defer instance.mu.Unlock()
  std::unique_lock<std::shared_mutex> lock(instance->mu);

	for (;!dead;) {
		if (!(instance->vd).empty()) {
			break;
		}
		(instance->p).np++;
		(instance->p).n = (instance->p).np;
    auto [ ok, value ] = propose(instance, seq);
		if (!ok) {
			continue;
		}
		if (!value.empty()) {
			v = value;
		}
		if (!request_accept(instance, seq, v)) {
			continue;
		}
		decide(seq, v);
		break;
	}
  return grpc::Status::OK;
}


// --------------------------- Testing Function ---------------------------

/* SimpleReceive service for testing */
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


/* entry point for running SimpleReceive service */
grpc::Status PaxosServiceImpl::SimpleStart(int seq, std::string v)
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
