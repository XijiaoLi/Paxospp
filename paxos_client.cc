#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "paxos.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using paxos::Paxos;
using paxos::EProposal;
using paxos::EResponse;

class PaxosClient {
    public:
        PaxosClient(std::shared_ptr<Channel> channel) : stub_(Paxos::NewStub(channel)) {}

        EResponse Echo(int seq, std::string value) {
            ClientContext context;
            EProposal proposal;
            EResponse response;

            proposal.set_seq(seq);
            proposal.set_value(value);

            Status status = stub_->Echo(&context, proposal, &response);

            if(status.ok()){
                return response;
            } else {
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                return response;
            }
        }

    private:
        std::unique_ptr<Paxos::Stub> stub_;
};

void Run() {
    std::string client_address("0.0.0.0:50051");
    PaxosClient pclient(
        grpc::CreateChannel(
            client_address,
            grpc::InsecureChannelCredentials()
        )
    );

    EResponse response;

    int seq = 1;
    std::string value = "hello";

    response = pclient.Echo(seq, value);
    std::cout << "Response received: seq = " << response.seq() << "; value = " << response.value() << std::endl;
}

int main(int argc, char* argv[]){
    Run();

    return 0;
}
