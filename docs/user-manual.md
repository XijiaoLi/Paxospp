## Code Building Blocks Introduction

### gRPC Services 
* **Paxos rpc:** 
    1. Ping: test if the server is available
    2. Receive: handles rpc requests based on the types

* **Messages:** 
    1. Proposal:
        ``string    type``               Type of the rpc message, propose/accept/decide, depending on the status of the server
        ``int32     proposed_num``       The proposal number
        ``int32     seq``                The n-th instance in the paxos log
        ``string    value``              The value server tries to store in the log
        ``int32     me``                 The ID of the server
        ``int32     done``               Finished instances
    2. Response:
        ``string    type``               Type of the rpc message
        ``bool      approved``           Whether or not the proposed_num satisfies conditions 
        ``int32     number``             The highestSeenProposedNumber from the peer server 
        ``string    value``              The value associated with the highestSeenProposedNumber
        ``int32     me``                 The ID of the server
        ``int32     done``               Finished instances

### struct
* **Instance:** The paxos instance, in other words, a log in the database, once the value stored is decided, the data is immutable.
    ```C++
    struct Instance {
        std::shared_mutex mu;
        Proposer p;
        Acceptor a;
        std::string vd;
    }
    ```

* **Proposer:** The struct stores the proposedNumber and highestSeenProposedNumber
    ```C++
    struct Proposer {
        int n;
        int np;
    };
    ```

* **Acceptor:** The struct stores the highestProposedNumber, highestAcceptedNumber, and highestAcceptedValue
    ```C++
    struct Acceptor {
        int np;
        int na;
        std::string va;
    };
    ```

### Classes
* **PaxosServiceImpl:** This class is the Paxos Protocol Implementation class, which will help servers to make agreement on logs
    * Members
        - Peers related
            ```C++
            int peers_num                                          Number of nodes in the system 
            std::vector<std::string> peers_addr                    Peer ip address and port number
            std::vector<std::shared_ptr<grpc::Channel>> channels   Grpc channels
            ```
        - Testing related 
            ```C++
            bool debug
            bool dead
            ```
        - Server and Database component
            ```C++
            int me                                                  ID of the node
            bool initialized                                        Initiation parameter
            std::unique_ptr<std::thread> listener                   Keep-alive Server listening all the time
            std::unique_ptr<grpc::Server> server                    Thread runs the lisenting server
            mutable std::shared_mutex mu                            Lock on the operations               
            mutable std::shared_mutex acceptor_lock                 Lock on the requests
            std::map<int, Instance*> instances                      Local replication stores the decided values 
            std::vector<std::future<bool>> request_threads          Placeholder for different requests        
            ```
            
    * Interfaces
        ```C++
        PaxosServiceImpl(std::vector<std::string> peers_addr, int me); // constructor
        void InitializeService(); // initialize server, channel, stub
        void StartService(); // start listening on the address and port
        void TerminateService(); // shut down the service on the server
        grpc::Status Start(int seq, std::string v); // main entry point for running paxos Receive service
        std::tuple<bool, std::string> Status(int seq); // check a paxos peer's decision on an instance
        ```