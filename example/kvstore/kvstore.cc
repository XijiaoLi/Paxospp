#include "kvstore.h"

namespace kvstore {

// ---------------------------- Helper Function ----------------------------

void RunServer() {

  std::map<std::string, std::string> db_seeds {
    {"1", "one"},
    {"2", "two"}
  };
  int paxos_num = 3;
  std::vector<std::string> paxos_addr {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053" };

  KVStoreImpl kv_service(db_seeds, paxos_num, paxos_addr, 0);

  std::string server_address("0.0.0.0:50061");
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "kv_service" as the instance through which we'll communicate with
  // clients. In this case, it corresponds to an *synchronous* kv_service.
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

  index = op_str.find(delimiter, prev);
  op.timestamp = std::stoll(op_str.substr(prev, index), nullptr, 10);
  prev = index;

  index = op_str.find(delimiter, prev);
  op.client_id = std::stoll(op_str.substr(prev, index), nullptr, 10);
  prev = index;

  index = op_str.find(delimiter, prev);
  op.type = op_str.substr(prev, index);
  prev = index;

  index = op_str.find(delimiter, prev);
  op.key = op_str.substr(prev, index);
  prev = index;

  index = op_str.find(delimiter, prev);
  op.value = op_str.substr(prev, index);
  prev = index;

  return op;
}

// ----------------------- KVStoreImpl Function -----------------------

/* Constructor */
KVStoreImpl::KVStoreImpl(std::map<std::string, std::string> db, int peers_num, std::vector<std::string> peers_addr, int me)
  : db(db), px(peers_num, peers_addr, me), px1(peers_num, peers_addr, 1), px2(peers_num, peers_addr, 2) {
    px.InitializeService();
    px.StartService();

    px1.InitializeService();
    px1.StartService();

    px2.InitializeService();
    px2.StartService();
  }


/* Put service for putting a value into db */
grpc::Status KVStoreImpl::Put(ServerContext* context, const KVRequest* request, KVResponse* response)
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

  auto [ err, val ] = write_log(op);

  std::cout << "kvStore server received PUT " << key << " with " << value << "\n";
  response->set_err(err);
  return grpc::Status::OK;
}

/* Get service for getting a value from db */
grpc::Status KVStoreImpl::Get(ServerContext* context, const KVRequest* request, KVResponse* response)
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

  auto [ err, val ] = write_log(op);

  std::cout << "kvStore server received GET " << key << " found " << val << "\n";
  response->set_err(err);
  response->set_value(val);
  return grpc::Status::OK;
}


std::tuple<std::string, std::string> KVStoreImpl::write_log(Op op)
{
    std::string op_str = encode(op);
    std::cout << "kvStore server encoded op " << op_str << "\n";

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
      px.Start(seq, op_str);
      Op returned_op = get_log(seq, op);
      auto [ err, value ] = execute_log(returned_op);
      latest_requests[returned_op.client_id] = new Response{returned_op.timestamp, err, value};
      committed_seq++;
      if (returned_op.client_id == op.client_id && returned_op.timestamp == op.timestamp) {
        return std::make_tuple(err, value);
      }
    }

    db[op.key] = op.value;
    return std::make_tuple("ok", "");
}


Op KVStoreImpl::get_log(int seq, Op op)
{
  bool end = false;
  std::string op_str;
  for (;!end;) {
    auto [decided, val] = px.Status(seq);
    end = decided;
    op_str = val;
    usleep(200);
    std::cout << "end = " << end << "\n";
  }
  return op;
}


std::tuple<std::string, std::string> KVStoreImpl::execute_log(Op op)
{
  std::string err = "", value = "";
  if (op.type == "PUT") {
    db[op.key] = op.value;
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
  std::cout << "execute log " << op.type << " " << err << " " << value << "\n";
  return std::make_tuple(err, value);
}


} // namespace kvstore


// void show_usage(char* name)
// {
//   std::cerr << "Usage: " << name << " <option(s)> \n"
//             << "Options:\n"
//             << "\t-h\tShow this help message\n"
//             << "\t-n\t<int>\tSpecify the index of kv server among its peers\n"
//             << "\t-p\t<address> <address>...\tSpecify the list of address of paxos servers\n"
//             << "\t-s\t<address>\tSpecify the address of kv server\n"
//             << std::endl;
// }
//
// bool legal_int(char *str) {
//   while (*str) {
//     if (!isdigit(*str++)) {
//       return false;
//     }
//   }
//   return true;
// }


int main(int argc, char** argv) {
  // std::cerr << argc << "\n";
  // if (argc < 1) {
  //   return -EINVAL;
  // }
  //
  // std::vector<std::string> paxos_addr;
  // std::string kvserver_addr;
  // int me, peers_num;
  //
  // int arg_i = 0;
  // while (arg_i < argc) {
  //   arg_i ++;
  //   if (strcmp(argv[arg_i], "-p") == 0) {
  //     arg_i ++;
  //     while (arg_i < argc && argv[arg_i][0] != '-') {
  //       paxos_addr.push_back(argv[arg_i]);
  //       arg_i++;
  //     }
  //     peers_num = paxos_addr.size();
  //     if (peers_num == 0) {
  //       show_usage(argv[0]);
  //       return 0;
  //     }
  //   } else if (strcmp(argv[arg_i], "-h") == 0) {
  //     show_usage(argv[0]);
  //     return 0;
  //   } else if (strcmp(argv[arg_i], "-s") == 0) {
  //     arg_i ++;
  //     if (arg_i < argc && argv[arg_i][0] != '-') {
  //       kvserver_addr = argv[arg_i];
  //     } else {
  //       show_usage(argv[0]);
  //       return 0;
  //     }
  //   } else if (strcmp(argv[arg_i], "-n") == 0) {
  //     arg_i ++;
  //     if (arg_i < argc && legal_int(argv[arg_i])) {
  //       me = atoi(argv[arg_i]);
  //     } else {
  //       show_usage(argv[0]);
  //       return 0;
  //     }
  //   }
  // }

  kvstore::RunServer();

  return 0;
}
