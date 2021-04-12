/**
 *  @file   kvstore-server.cc
 *  @brief  KVStoreServer Implementation
 *  This file contains the function definitions for the
 *  KVStoreServer and some useful helper functions.
 *
 *  @author Xijiao Li
 *  @date   2021-04-12
 ***********************************************/

#include "kvstore-server.h"

namespace kvstore {

// ---------------------------- Helper Function ---------------------s-------

void RunServer(std::vector<std::string> paxos_addr, std::string kvserver_addr, int me) {

  std::map<std::string, std::string> db_seeds {
    {"1", "one"},
    {"2", "two"}
  };

  if (paxos_addr.size() == 0) {
    paxos_addr = {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };
  }

  KVStoreServer kv_service(db_seeds, paxos_addr, me);

  std::string server_address(kvserver_addr);
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "kv_service" as the instance through which we'll communicate with
  // clientop_str. In this case, it corresponds to an *synchronous* kv_service.
  builder.RegisterService(&kv_service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

std::string encode(Op op) {
  std::string res = std::to_string(op.timestamp) + "," + std::to_string(op.client_id) + "," + op.type + "," + op.key + "," + op.value;
  return res;
}

Op decode(std::string op_str) {
  Op op;

  std::string delimiter = ",";
  size_t prev = 0, index = 0;

  index = op_str.find(delimiter);
  op.timestamp = std::stoll(op_str.substr(prev, index), nullptr, 10);
  op_str.erase(0, index + 1);

  index = op_str.find(delimiter);
  op.client_id = std::stoll(op_str.substr(prev, index), nullptr, 10);
  op_str.erase(0, index + 1);

  index = op_str.find(delimiter);
  op.type = op_str.substr(prev, index);
  op_str.erase(0, index + 1);

  index = op_str.find(delimiter);
  op.key = op_str.substr(prev, index);
  op_str.erase(0, index + 1);

  index = op_str.find(delimiter);
  op.value = op_str.substr(prev, index);
  op_str.erase(0, index + 1);

  return op;
}

// ----------------------- KVStoreServer Function -----------------------

/* Constructor */
KVStoreServer::KVStoreServer(std::map<std::string, std::string> db, std::vector<std::string> peers_addr, int me)
  : db(db), px(peers_addr, me, false), committed_seq(0) {
    px.InitializeService();
    px.StartService();
  }


/* Put service for putting a value into db */
grpc::Status KVStoreServer::Put(ServerContext* context, const KVRequest* request, KVResponse* response)
{

  std::string key = request->key();
  std::string value = request->value();
  int64_t timestamp = request->timestamp();
  int64_t client_id = request->client_id();

  std::unique_lock<std::shared_mutex> lock(mu);

  Op op = {
    timestamp,
    client_id,
    "PUT",
    key,
    value
  };

  std::cout << "KV server recv: PUT (" << key << ", " << value << "), starting paxos...\n";

  auto [ err, val ] = write_log(op);
  response->set_err(err);
  return grpc::Status::OK;
}

/* Get service for getting a value from db */
grpc::Status KVStoreServer::Get(ServerContext* context, const KVRequest* request, KVResponse* response)
{
  std::string key = request->key();
  int64_t timestamp = request->timestamp();
  int64_t client_id = request->client_id();

  std::unique_lock<std::shared_mutex> lock(mu);

  Op op = {
    timestamp,
    client_id,
    "GET",
    key,
    ""
  };

  std::cout << "KV server recv: GET (" << key << "), starting paxos...\n";

  auto [ err, val ] = write_log(op);

  if (err.compare("NotFound") == 0) {
    std::cout << "\t  " << err << "\n";
  } else {
    std::cout << "\t  found value " << val << std::endl;
  }

  response->set_err(err);
  response->set_value(val);
  return grpc::Status::OK;
}


std::tuple<std::string, std::string> KVStoreServer::write_log(Op op)
{
    std::string op_str = encode(op);

    Response* latest_response;
    std::map<int64_t, Response*>::iterator it;
    it = latest_requests.find(op.client_id);
    if (it != latest_requests.end()) {
      latest_response = it->second;
      if (op.timestamp == latest_response->timestamp) {
        return std::make_tuple(latest_response->err, latest_response->value);
      } else if (op.timestamp < latest_response->timestamp) {
        return std::make_tuple("OutdatedRequest", "");
      }
    }

    for (;;) {
      int seq = committed_seq + 1;
      std::cout << "KV server chose seq " << seq << "\n";
      px.Start(seq, op_str);
      Op returned_op = get_log(seq);
      auto [ err, value ] = execute_log(returned_op);
      latest_requests[returned_op.client_id] = new Response{returned_op.timestamp, err, value};
      committed_seq++;
      if (returned_op.client_id == op.client_id && returned_op.timestamp == op.timestamp) {
        return std::make_tuple(err, value);
      }
    }

    db[op.key] = op.value;
    return std::make_tuple("OK", "");
}


Op KVStoreServer::get_log(int seq)
{
  bool end = false;
  std::string op_str;
  for (;!end;) {
    auto [decided, val] = px.Status(seq);
    end = decided;
    op_str = val;
    usleep(200);
  }
  Op op = decode(op_str);
  return op;
}


std::tuple<std::string, std::string> KVStoreServer::execute_log(Op op)
{
  std::string err = "", value = "";
  if (op.type.compare("PUT") == 0) {
    db[op.key] = op.value;
    err = "OK";
  } else {
    std::map<std::string, std::string>::iterator it;
    it = db.find(op.key);
    if (it != db.end()) {
      value = it->second;
      err = "OK";
    } else {
      err = "NotFound";
    }
  }
  std::cout << "KV server exec: [" << op.timestamp << "," << op.client_id << "] " << op.type << " " << op.key << " " << op.value
            << "\n\t  status = " << err << std::endl;

  return std::make_tuple(err, value);
}


} // namespace kvstore


void show_usage(char* name)
{
  std::cerr << "Usage: " << name << " <option(s)> \n"
            << "Options:\n"
            << "\t-h\tShow this help message\n"
            << "\t-i\t<int>\tSpecify the index of KV server's paxos among its peers\n"
            << "\t-p\t<address> <address>...\tSpecify the list of address of paxos servers\n"
            << "\t-s\t<address>\tSpecify the address of KV client will sent request to\n"
            << std::endl;
}

bool legal_int(char *str) {
  while (*str) {
    if (!isdigit(*str++)) {
      return false;
    }
  }
  return true;
}


int main(int argc, char** argv) {
  if (argc < 1) {
    return -EINVAL;
  }

  std::vector<std::string> paxos_addr;
  std::string kvserver_addr = "0.0.0.0:50061";
  int me = 0, peers_num;

  int arg_i = 1;
  while (arg_i < argc) {
    if (strcmp(argv[arg_i], "--paxos") == 0) {
      arg_i ++;
      while (arg_i < argc && argv[arg_i][0] != '-') {
        paxos_addr.push_back(argv[arg_i]);
        arg_i++;
      }
      arg_i--;
      peers_num = paxos_addr.size();
      if (peers_num == 0) {
        show_usage(argv[0]);
        return 0;
      }
    } else if (strcmp(argv[arg_i], "-h") == 0) {
      show_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[arg_i], "--address") == 0) {
      arg_i ++;
      if (arg_i < argc && argv[arg_i][0] != '-') {
        kvserver_addr = argv[arg_i];
      }
    } else if (strcmp(argv[arg_i], "-i") == 0) {
      arg_i ++;
      if (arg_i < argc && legal_int(argv[arg_i])) {
        me = std::stoi(argv[arg_i]);
      }
    }
    arg_i++;
  }

  kvstore::RunServer(paxos_addr, kvserver_addr, me);

  return 0;
}
