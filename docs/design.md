
# Designs
## Code Directory Introduction

**lib:** This directory includes all head files and all implementation of Paxospp, You can figure out the working principle of Paxoscpp by reading this directorys. No neccessary to read it if you are only using Paxospp.

**protos:** This directory includes all proto files required by gRPC communication.

**test:** This directory includes all the test codes and test datasets.

**examples:** This directory provides an example usage of Paxoscpp.

**include:** This directory contains all the header files by Paxospp.

## Code Building Blocks Introduction

### Classes
* **PaxosServiceImpl:** This class is the Paxos Protocol Implementation class, which will help servers to make agreement on logs

### gRPC Services 

* **Paxos rpc:** 
    1. Ping: test if the server is available
    2. Receive: handles rpc requests based on the types

* **Messages:** 
    1. Proposal:
        1. string    type               Type of the rpc message, propose/accept/decide, depending on the status of the server
        2. int32     proposed_num       The proposal number
        3. int32     seq                The n-th instance in the paxos log
        4. string    value              The value server tries to store in the log
        5. int32     me                 The ID of the server
        6. int32     done               Finished instances
    2. Response:
        1. string    type               Type of the rpc message
        2. bool      approved           Whether or not the proposed_num satisfies conditions 
        3. int32     number             The highestSeenProposedNumber from the peer server 
        4. string    value              The value associated with the highestSeenProposedNumber
        5. int32     me                 The ID of the server
        6. int32     done               Finished instances

### struct
* **Instance:** The paxos instance, in other words, a log in the database, once the value stored is decided, the data is immutable.
* **Proposer:** The struct stores the proposedNumber and highestSeenProposedNumber
* **Acceptor:** The struct stores the highestProposedNumber, highestAcceptedNumber, and highestAcceptedValue

## Choices
* **gRPC integration benefits:**
    1. The contract can be auto generated from .proto file, and the rpc messages are easily defined and used.
    2. HTTP/2 is serving under the hood of gRPC, by reusing the channels in our code, less connections are made, hence less RTTs, and less time to reach the agreements between different servers.
    3. Light weight gRPC client.


# Features

# Limitations

# Performance


### Setup

    CPU: Intel(R) Core(TM) i7-4980HQ CPU @ 2.80GHz
    Memory: 16 GB
    Disk: ssd
    Network: Gigabit Ethernet
    Cluster Nodes: 3???
    Ping: 0.05ms???
    Parallel client: 100 Threads???

### Performance Test Result(QPS)

> Request latency small than 10ms.

###### Data set with small size(100B)

    1 Group: 1171
    20 Groups: 11931
    50 Groups: 13424
    100 Groups: 13962

###### Data set with larse size(100KB)

    1 Group: 280
    20 Groups: 984
    50 Groups: 1054
    100 Groups: 1067

###### BatchPropose(2KB)

    100 Groups: 150000

### Performance Test Result(QPS)


