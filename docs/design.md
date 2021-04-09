
# Design Documentation

## Overview
--------
Paxospp aims to create a lightweight Paxos consensus protocol library that has a simple and easy-to-use interface. This libaray helps the programmers to develop multi-server applications without dealing with all the system, networks, and other reliability issues. It helps services in synchronizing the state from a single node to the other nodes to form a multi-copy cluster and handling fail-over automatically. A typical application using our libaray would be distributed data storage system.

### Agreement in a Paxospp
- Majority agreement
Paxospp Library requires only a majority of nodes to be alive and make sequntial progress. For nodes in the distributed environment, we  recognize the complexities of the situation, it's both more time efficient and relible to just make a few nodes agree. In the real world, there could be server crashes or network unreliablities. If a minority of the nodes is disconnected from the other servers, the paxospp service continues to function and store the intended values. Later when the defective server reboots or the network recovers, the minority of nodes can catch up. Hence consistency is maintained.

### Communication between the servers
- Network protocol
Paxospp uses gRPC under the hood. gRPC is build on http2 and TCP protocol. In Paxospp, integrity is provided in different layers. During the network communication, Paxospp offers data integrity and delivery gurantee. Also, by applying features of the network protocol saves RTTs and enables Paxospp library to use less time overall in the logging process.

## Design Choices
--------
### Multithreading and Concurrency
- Node representation
A Paxospp node in Paxospp library is naturally divided and represented by two modules, a server component which keeps listening for the incoming synchronization requests and a client part which initiates data storage requests and passes to other nodes. Each server is on a thread listening for requests.

- Concurrent Data Storage
Multiple instnaces may be proposed from different nodes around the same time. Paxospp runs the protocol concurrently for all of these instances. Paxospp continues to make progress toward agreement for all of these instances. We choose to start a new thread for each instance hence offer a non-blocking service.

### Smart pointers
- Unique pointers
Unique pointers are heavily used in Paxospp library. Because `PaxosServiceImpl` destructor will recursively call its members' destructors, and it is reasonable to release its resources when the object is out of scope. Smartpointer enables us to achive the automatic resource release without overriding default destructor and reimplement the normal pointer release process.

### Locking
- Shared_mutex
Paxospp protects coherence among nodes by using locks before and after checking or changing the database in the node. So peers will not get chaotic views. 

## Features

### Easy to use
- Paxospp has a simple and clean interface to use. A Kvstore example is also available for users to dive deeper and learn how to use Paxospp.

### Fast and fault tolorance
- Paxospp library makes progress even when a subset of the nodes is up. In consideration of the complex environment in the real world, Paxospp tolerates the server failures and network crashes while still maintains data consistency.

### Concurency
- Paxsospp enables multiple instances to launch at the same time in a non-blocking fashine. 

## Future works

### Enable Polymorphism Storage
- Currently, we only allows `std::string` stored in the paxos database. We would like to change to `std::any` when it is available in the future when new C++ versions released.

### Space Saving
- In the current version, Paxospp stores all the previous data. However, some data may not needed after a certain amount of time, and it will just be a waste of disk resources to continue storing them. 


