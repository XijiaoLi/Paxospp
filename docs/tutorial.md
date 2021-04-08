# Tutorial

## Installing Paxos-Cpp Library

### Install Third Party Libs
Paxospp depends on gRPC and Protocol Buffers. The steps in the section explain now to build and locally install gRPC and Protocol Buffers using `cmake`. If you’d rather use [bazel](https://www.bazel.build/), see [Building from source](https://github.com/grpc/grpc/blob/master/BUILDING.md#build-from-source).

#### Setup

Choose a directory to hold locally installed packages. This page assumes that the environment variable `MY_INSTALL_DIR` holds this directory path. For example:

```sh
$ export MY_INSTALL_DIR=$HOME/.local
```

Ensure that the directory exists:

```sh
$ mkdir -p $MY_INSTALL_DIR
```

Add the local `bin` folder to your path variable, for example:

```sh
$ export PATH="$PATH:$MY_INSTALL_DIR/bin"
```

#### Install cmake

You need version 3.13 or later of `cmake`. Install it by following these instructions:

- Linux: `$ sudo apt install -y cmake`

- macOS: `$ brew install cmake`


For general `cmake` installation instructions, see [Installing CMake](https://cmake.org/install).

Check the version of `cmake`:

```sh
$ cmake --version
cmake version 3.19.7
```

Under Linux, the version of the system-wide `cmake` can often be too old. You can install a more recent version into your local installation directory as follows:

```sh
$ wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.19.6/cmake-3.19.6-Linux-x86_64.sh
$ sh cmake-linux.sh -- --skip-license --prefix=$MY_INSTALL_DIR
$ rm cmake-linux.sh
```

#### Install other required tools

Install the basic tools required to build gRPC:

- Linux: `$ sudo apt install -y build-essential autoconf libtool pkg-config`

- macOS: `$ brew install autoconf automake libtool pkg-config`

#### Clone the `grpc` repo

Clone the `grpc` repo and its submodules:

```sh
$ git clone --recurse-submodules -b v1.35.0 https://github.com/grpc/grpc
```

#### Build and install gRPC and Protocol Buffers

While not mandatory, gRPC applications usually leverage [Protocol Buffers](https://developers.google.com/protocol-buffers) for service definitions and data serialization, and the example code uses [proto3](https://developers.google.com/protocol-buffers/docs/proto3).

The following commands build and locally install gRPC and Protocol Buffers:

```sh
$ cd grpc
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
$ make -j
$ make install
$ popd
```

#### Important notes

We **strongly** encourage you to install gRPC *locally* — using an appropriately set `CMAKE_INSTALL_PREFIX` — because there is no easy way to uninstall gRPC after you’ve installed it globally.

If errors and warnings were encountered when you followed exactly the scripts above, there are two solutions:
1. It is possible that you switched to a different dir when installing
2. Or your system is out of memory/resources, try clean up some unused resources and follow the script again

### Build and run Paxos server

Clone this `Paxos-Cpp` repo:

```sh
$ https://github.com/XijiaoLi/Paxos-Cpp.git
$ cd Paxos-Cpp
```

Go to the `Paxos-Cpp/example/` folder, and run the following commands to build the excutable using `cmake`:

```sh
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../../test  
$ make -j
```
Note: the last parameter of the cmake command is the directory contains the CMakeLists.txt. If Errors saying the CMakeLists.txt doesn't match, do the following and run the above commands again:

```sh
$ cd ../..
$ rm -r cmake/build
```

Now you should be at **build** directory `Paxos-Cpp/example/cmake/build`, with the excutable `server`.

Run the server:

```bash
$ ./server
Adding 0.0.0.0:50051 to the channel list ...
Adding 0.0.0.0:50052 to the channel list ...
Adding 0.0.0.0:50053 to the channel list ...
...
```

## How to Wrap Your Own Code Around Paxospp
Your implementation must support this interface:

```C++
 PaxosServiceImpl paxos_0(int replica_size, std::vector<std::shared_ptr<grpc::Channel>> channels, int me); 
 paxos_0.Start(int seq, std::string v); // start agreement on new instance
 std::tuple<bool, std::string> paxos_0.Status(seq int); // get info about an instance
```

An application calls PaxosServiceImpl() Constructor to create a Paxos peer. The channels argument contains the address and ports of all the peers (including this one), and the me argument is the index of this peer in the peers array. Start(int seq, std::string v) asks Paxos to start agreement on instance seq, with proposed value v; Start() should return immediately, without waiting for agreement to complete. The application calls Status(seq) to find out whether the Paxos peer thinks the instance has reached agreement, and if so what the agreed value is. Status() should consult the local Paxos peer’s state and return immediately; it should not communicate with other peers. The application may call Status() for old instances.

Your implementation should be able to make progress on agreement for multiple instances at the same time. That is, if application peers call Start() with different sequence numbers at about the same time, your implementation should run the Paxos protocol concurrently for all of them. You should not wait for agreement to complete for instance i before starting the protocol for instance i+1. Each instance should have its own separate execution of the Paxos protocol.

## A First Project

Let's begin the simplest project with Paxos-cpp libaray

```sh
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../../helloworld  
$ make -j
```

