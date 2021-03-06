/**
 *  @file   paxos.cc
 *  @brief  Paxos Service Implementation
 *  This file contains the function implementations for the Paxos Class.
 *
 *  @author Xijiao Li
 *  @date   2021-04-12
 ***********************************************/


#include <cstdlib>
#include <algorithm>

#include "paxos.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;

using paxos::PaxosService;
using paxos::Proposal;
using paxos::Response;
using paxos::EmptyMessage;

namespace paxos {

// --------------------- Paxos Public Function ----------------------

// Constructor
Paxos::Paxos(std::vector<std::string> peers_addr, int me, bool debug)
  : peers_num(peers_addr.size()), peers_addr(peers_addr), me(me), dead(false), initialized(false), debug(debug) {}

Paxos::Paxos(std::vector<std::string> peers_addr, int me)
  : peers_num(peers_addr.size()), peers_addr(peers_addr), me(me), dead(false), initialized(false), debug(false) {}


// Shut down the server
void Paxos::TerminateService()
{
  std::unique_lock<std::shared_mutex> lock(mu);
  std::cout << "Server shutdown..." << std::endl;
  dead = true;
  server->Shutdown();
  listener->join();
}

// Initialize Paxos Service
void Paxos::InitializeService()
{
  if (!initialized) {
    grpc::ServerBuilder builder;
    // listen on the given address
    builder.AddListeningPort(peers_addr[me], grpc::InsecureServerCredentials());
    // register "service" as the instance to communicate with clients
    builder.RegisterService(this);
    // assemble the server
    server = std::move(builder.BuildAndStart());
    // wait for the server to shutdown
    if (debug){
      std::cout << "Paxos is now listening on: " << peers_addr[me] << std::endl;
    }

    // construct channels and stubs
    for (int i = 0; i < peers_num; ++i) {
      // at each endpoint, create a channel for paxos to send rpc, and create a stub associated with the channel
      std::shared_ptr<grpc::Channel> channel_i = grpc::CreateChannel(peers_addr[i], grpc::InsecureChannelCredentials());
      std::unique_ptr<PaxosService::Stub> peer_i = std::make_unique<PaxosService::Stub>(channel_i);
      channels.push_back(std::move(channel_i));
      peers.push_back(std::move(peer_i));
      if (debug){
        std::cout << "Adding peer " << i << " with address " << peers_addr[i] << " to the channel/stub list ..." << std::endl;
      }
    }
  }
}


// Server starts to listen on the address
void Paxos::StartService()
{
  listener = std::make_unique<std::thread>([this]() {start_service();});
}


// Ping service for checking aliveness
grpc::Status Paxos::Ping(ServerContext* context, const EmptyMessage* request, EmptyMessage* response)
{
  return grpc::Status::OK;
}


// Receive service for communication between paxos peers
grpc::Status Paxos::Receive(ServerContext* context, const Proposal* proposal, Response* response)
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

    if (debug){
        std::cout << "SPaxos " << me << " from CPaxos " << peer
                  << "\n\t DECIDE (seq, val) = (" << seq << ", " << value << ")"
                  << " from initial value \'" << instance->vd << "\'" << std::endl;
    }

    instance->vd = value;
    response->set_approved(true);
  }
  return grpc::Status::OK;
}


/** Check whether a peer thinks an instance has been decided,
 *  and if so what the agreed value is. Should just inspect
 *  the local peer state; it should not contact other peers.
 */
std::tuple<bool, std::string> Paxos::Status(int seq)
{
  std::unique_lock<std::shared_mutex> lock(mu);

  bool decided;
  std::string val;

  std::map<int, Instance*>::iterator it;
  Instance* instance;


  it = instances.find(seq);
  if (it != instances.end()) {
    decided = true;
    instance = it->second;
    val = instance->vd;
    decided = val.compare("") != 0 ? true : false;
  } else {
    decided = false;
    val = "";
  }
  if (debug){
    std::string decided_print = decided == true ? "True":"False";
    std::cout << me << "-" << decided << ":" << val << std::endl;
  }
  return std::make_tuple(decided, val);
}


/**
 * The application wants paxos to start agreement on
 * instance seq, with proposed value v.
 * Start() returns right away; the application will
 * call Status() to find out if/when agreement
 * is reached.
 */
grpc::Status Paxos::Start(int seq, std::string v)
{
  request_threads.push_back(std::async(std::launch::async,&Paxos::start, this, seq, v));
  return grpc::Status::OK;
}


// --------------------- Paxos Private Function ---------------------

/// Inner function for starting service
void Paxos::start_service()
{
  if (debug){
     std::cout << "Wait for the server to shutdown..." << std::endl;
  }
  server->Wait();
}


/// Inner function for Start paxos
bool Paxos::start(int seq, std::string v)
{
  Instance* instance = get_instance(seq);

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
  return true;
}


/// Get the instance with the given seq number, or create one and return the pointer to it if not found
Instance* Paxos::get_instance(int seq)
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


///
std::tuple<bool, std::string> Paxos::propose(Instance* instance, int seq)
{
  int count = 0;
  int highest_np = (instance->p).np;
  int highest_na = -1;
  std::string highest_va;

  int i = 0; // only for logging usage
  for (const auto& stub : peers) {

    ClientContext context;

    Proposal proposal;
    proposal.set_type("PROPOSE");
    proposal.set_proposed_num((instance->p).n);
    proposal.set_seq(seq);
    proposal.set_value("");
    proposal.set_me(me);
    proposal.set_done(0);

    Response response;
    if (debug){
      std::cout << "CPaxos " << me << " sent PROPOSE to SPaxos " << i << std::endl;
    }

    grpc::Status status = stub->Receive(&context, proposal, &response);
    if (!status.ok()) {
      continue;
    }

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


bool Paxos::request_accept(Instance* instance, int seq, std::string v)
{
  int highest_np = (instance->p).np;
  int count = 0;

  int i = 0;
  for (const auto& stub : peers) {

    ClientContext context;

    Proposal proposal;
    proposal.set_type("ACCEPT");
    proposal.set_proposed_num((instance->p).n);
    proposal.set_seq(seq);
    proposal.set_value(v);
    proposal.set_me(me);
    proposal.set_done(0);

    Response response;

    if (debug){
      std::cout << "CPaxos " << me << " sent ACCEPT to SPaxos " << i << std::endl;
    }

    grpc::Status status = stub->Receive(&context, proposal, &response);

    if (status.ok()) {
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


void Paxos::decide(int seq, std::string v)
{
  std::vector<bool> records (peers.size(), false);
  int count = 0;

  for (; count < peers_num;) {
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

      grpc::Status status = stub->Receive(&context, proposal, &response);

      if (!status.ok()) {
        if (debug){
          std::cout << "CPaxos " << me << " got from SPaxos " << i << " FAILED!" << std::endl;
        }
      } else {
        bool approved = response.approved();
        int peer = response.me();
        int peer_done = response.done();
        if (debug){
          std::cout << "CPaxos " << me << " got from SPaxos " << i << ", DECIDE approved = " << approved << std::endl;
        }
        if (approved) {
          records[i] = true;
          count ++;
        }
        i++;
      }
    }
  }
  return;
}

} // end of namespace paxos
