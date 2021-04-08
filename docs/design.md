
# Designs
## Code Directory Introduction

**lib:** This directory includes all head files and all implementation of Paxospp, You can figure out the working principle of Paxoscpp by reading this directorys. No neccessary to read it if you are only using Paxospp.

**protos:** This directory includes all proto files required by gRPC communication.

**test:** This directory includes all the test codes and test datasets.

**examples:** This directory provides an example usage of Paxoscpp.

**include:** This directory contains all the header files by Paxospp.

## Code Building Blocks Introduction



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


