#include <unistd.h>
#include "kvstore.h"

class KVStoreClient
{
  public:
    KVStoreClient(std::shared_ptr<grpc::Channel> channel)
      : stub(KVStore::NewStub(channel)) {}

    std::string Put(const std::string& key, const std::string& value);

    std::string Get(const std::string& key, const std::string& val);

  private:
    std::unique_ptr<KVStore::Stub> stub;
};
