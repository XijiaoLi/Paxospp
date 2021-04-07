#include <unistd.h>
#include "kvstore.h"

using grpc::ClientContext;
using grpc::Status;

using kvstore::KVStore;
using kvstore::KVRequest;
using kvstore::KVResponse;

namespace kvstore {

class KVStoreClient {
 public:
  KVStoreClient(std::shared_ptr<grpc::Channel> channel)
      : stub(KVStore::NewStub(channel)) {}

  std::string Put(const std::string& key, const std::string& value) {
    ClientContext context;

    KVRequest request;
    request.set_key(key);
    request.set_value(value);

    KVResponse response;

    std::cout << "kvStore client sent PUT " << key << " with " << value << "\n";

    Status status = stub->Put(&context, request, &response);

    if (status.ok()) {
      std::cout << "kvStore client received PUT " << key << " with " << value << " succeeded\n";
    } else {
      std::cout << "kvStore client received PUT " << key << " with " << value << " failed\n";
    }

    return response.err();
  }

  std::string Get(const std::string& key, const std::string& val) {
    ClientContext context;

    KVRequest request;
    request.set_key(key);

    KVResponse response;

    std::cout << "kvStore client sent GET " << key << "\n";

    Status status = stub->Get(&context, request, &response);

    if (status.ok()) {
      std::cout << "kvStore client received GET " << key << " with " << response.value() << " succeeded\n";
    } else {
      std::cout << "kvStore client received GET " << key << " failed\n";
    }

    return response.value();
  }

 private:
  std::unique_ptr<KVStore::Stub> stub;
};

} // namespace kvstore

int main(int argc, char** argv) {

  kvstore::KVStoreClient client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials())
  );
  std::string key("hello");
  std::string value("world");

  std::string reply;
  reply = client.Put(key, value);
  usleep(5000000);
  reply = client.Get(key, value);
  std::cout << "KVStore received: " << reply << std::endl;

  return 0;
}
