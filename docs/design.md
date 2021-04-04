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

# 