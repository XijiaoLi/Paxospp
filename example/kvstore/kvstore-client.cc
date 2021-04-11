#include "kvstore-client.h"

using grpc::ClientContext;
using grpc::Status;

using kvstore::KVStore;
using kvstore::KVRequest;
using kvstore::KVResponse;
using namespace std::chrono;

namespace kvstore {

KVStoreClient::KVStoreClient(std::shared_ptr<grpc::Channel> channel)
  : stub(KVStore::NewStub(channel)) {}

std::tuple<std::string, std::string> KVStoreClient::Put(const std::string& key, const std::string& value)
{
  ClientContext context;

  KVRequest request;
  long long ts = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  request.set_key(key);
  request.set_value(value);
  request.set_timestamp(ts);
  request.set_client_id(1);

  KVResponse response;

  std::cout << "KV client sent: PUT (" << key << ", " << value << ")\n";
  Status status = stub->Put(&context, request, &response);

  if (!status.ok()) {
    std::cout << "KV client recv: PUT (" << key << ", " << value << ") failed with grpc err " << status.error_code() << "\n";
  }

  return std::make_tuple(response.err(), response.value());
}

std::tuple<std::string, std::string> KVStoreClient::Get(const std::string& key, const std::string& val)
{
  ClientContext context;

  KVRequest request;
  long long ts = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  request.set_key(key);
  request.set_timestamp(ts);
  request.set_client_id(1);

  KVResponse response;

  std::cout << "KV client sent: GET (" << key << ")\n";
  Status status = stub->Get(&context, request, &response);

  if (!status.ok()) {
    std::cout << "KV client recv: GET (" << key << ") failed with grpc error " << status.error_code() << "\n";
  }

  return std::make_tuple(response.err(), response.value());
}

} // namespace KVstore

void show_usage(char* name)
{
  std::cerr << "Usage: " << name << " <option(s)> \n"
            << "Options:\n"
            << "\t-h\tShow this help message\n"
            << "\t-a\t<address>\tSpecify the address of KV client will sent request to\n"
            << "\t-t\t<type>\tSpecify the type of action in the operationt\n"
            << "\t-k\t<key>\tSpecify the key in the operation\n"
            << "\t-v\t<value>\tSpecify the value in the operation\n"
            << std::endl;
}

int main(int argc, char** argv) {

  std::string KVserver_addr = "0.0.0.0:50061", key = "hello", val = "world", type = "PUT";

  int arg_i = 1;
  while (arg_i < argc) {
    if (strcmp(argv[arg_i], "-h") == 0) {
      show_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[arg_i], "--address") == 0) {
      arg_i ++;
      if (arg_i < argc && argv[arg_i][0] != '-') {
        KVserver_addr = argv[arg_i];
        arg_i++;
      }
    } else if (strcmp(argv[arg_i], "--key") == 0) {
      arg_i ++;
      if (arg_i < argc && argv[arg_i][0] != '-') {
        key = argv[arg_i];
        arg_i++;
      }
    } else if (strcmp(argv[arg_i], "--value") == 0) {
      arg_i ++;
      if (arg_i < argc && argv[arg_i][0] != '-') {
        val = argv[arg_i];
        arg_i++;
      }
    } else if (strcmp(argv[arg_i], "--type") == 0) {
      arg_i ++;
      if (arg_i < argc && argv[arg_i][0] != '-') {
        type = argv[arg_i];
        arg_i++;
      }
    }
  }

  kvstore::KVStoreClient client(grpc::CreateChannel(
      KVserver_addr, grpc::InsecureChannelCredentials())
  );

  if (type.compare("PUT") == 0) {
    auto [err, value] = client.Put(key, val);
    if (err.compare("OK") == 0) {
      std::cout << "KV Client recv: " << err << std::endl;
    } else {
      std::cout << "KV Client recv: error " << err << std::endl;
    }
  } else {
    auto [err, value] = client.Get(key, val);
    if (err.compare("NotFound") == 0) {
      std::cout << "KV Client recv: error " << err << std::endl;
    } else {
      std::cout << "KV Client recv: " << value << std::endl;
    }
  }

  return 0;
}
