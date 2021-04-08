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