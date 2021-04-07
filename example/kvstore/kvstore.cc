#include "kvstore.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using kvstore::KVStore;
using kvstore::KVRequest;
using kvstore::KVResponse;

namespace kvstore {

// ----------------------- KVStoreImpl Function -----------------------

/* Constructor */
KVStoreImpl::KVStoreImpl(std::map<std::string, std::string> db)
  : db(db) {}

/* Put service for putting a value into db */
grpc::Status KVStoreImpl::Put(ServerContext* context, const KVRequest* request, KVResponse* response)
{
  std::string key = request->key();
  std::string value = request->value();
  db[key] = value;

  std::cout << "kvStore server received PUT " << key << " with " << value << "\n";
  response->set_err("ok");
  response->set_value("ok");
  return grpc::Status::OK;
}

/* Get service for getting a value from db */
grpc::Status KVStoreImpl::Get(ServerContext* context, const KVRequest* request, KVResponse* response)
{
  std::string key = request->key();
  std::string value;

  std::map<std::string, std::string>::iterator it;

  it = db.find(key);
  if (it != db.end()) {
    value = it->second;
    response->set_err("ok");
    response->set_value(value);
  } else {
    response->set_err("not found");
  }

  std::cout << "kvStore server received GET " << key << " found " << response->value() << "\n";
  return grpc::Status::OK;
}


// ---------------------------- Helper Function ----------------------------

void RunServer() {

  std::map<std::string, std::string> db_seeds {
    {"1", "one"},
    {"2", "two"}
  };
  KVStoreImpl service(db_seeds);

  std::string server_address("0.0.0.0:50051");
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case, it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

} // namespace kvstore


int main() {
  kvstore::RunServer();

  return 0;
}
