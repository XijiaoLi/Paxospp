# Tutorial

### Introduction

Paxospp is the lightweight C++ implementation of Lamport's Paxos algorithm. This page will help you install all the packages you need, and illurstate how you can work with Paxospp.


### Working with Paxospp Library

#### Install Third Party Libs
Paxospp depends on gRPC and Protocol Buffers. The steps in the section explain now to build and locally install gRPC and Protocol Buffers using `cmake`. If you’d rather use [bazel](https://www.bazel.build/), see [Building from source](https://github.com/grpc/grpc/blob/master/BUILDING.md#build-from-source).

*Setup*

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

*Install cmake*

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

*Install other required tools*

Install the basic tools required to build gRPC:

- Linux: `$ sudo apt install -y build-essential autoconf libtool pkg-config`

- macOS: `$ brew install autoconf automake libtool pkg-config`

##### Clone the `grpc` repo

Clone the `grpc` repo and its submodules:

```sh
$ git clone --recurse-submodules -b v1.35.0 https://github.com/grpc/grpc
```

##### Build and install gRPC and Protocol Buffers

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

*** Important notes

We strongly encourage you to install gRPC *locally* — using an appropriately set `CMAKE_INSTALL_PREFIX` — because there is no easy way to uninstall gRPC after you’ve installed it globally.

If errors and warnings were encountered when you followed exactly the scripts above, there are two solutions:
1. It is possible that you switched to a different dir when installing
2. Or your system is out of memory/resources, try clean up some unused resources and follow the script again

#### Build Paxospp library

Clone this `Paxospp` repo:

```sh
$ https://github.com/XijiaoLi/Paxospp.git
$ cd Paxospp
```

Under this  `Paxospp` folder, and run the following commands to build the excutable using `cmake`:

```sh
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
$ make -j
```
Now you should be at **build** directory `Paxospp/cmake/build`, with the static library  `libpaxos.a` file.



### How to Wrap Your Own Code Around Paxospp

#### A First Project: KVStore

Let's begin the simplest project with Paxos-cpp libaray.
Go to the `Paxospp/example/kvstore/` folder, and run the following commands to build the excutable using `cmake`:

```sh
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
$ make -j
```

Now you should be at **build** directory `Paxospp/kvstore/cmake/build`, with the excutable `kvstore-server` and `kvstore-client`.

Open three terminal tabs to run the servers at different addresses, and open another terminal tab to run the client. Run ``./kvstore-server -h` or `./kvstore-client -h` to see which options to pass in.
Below is the sample output of one of the three server and the client.

First server:

```bash
$ ./kvstore-server --paxos 0.0.0.0:50050 0.0.0.0:50051 -i 0 --address 0.0.0.0:50060
Server listening on 0.0.0.0:50060
KV server recv: PUT (boo, foo), starting paxos...
KV server chose seq 1
KV server exec: [1618279368824,1] PUT boo foo
	  status = OK
...
```

Second server:

```bash
$ ./kvstore-server --paxos 0.0.0.0:50050 0.0.0.0:50051 -i 1 --address 0.0.0.0:50061
Server listening on 0.0.0.0:50061
KV server recv: GET (boo), starting paxos...
KV server chose seq 1
KV server exec: [1618279368824,1] PUT boo foo
	  status = OK
KV server chose seq 2
KV server exec: [1618279413474,1] GET boo
	  status = OK
	  found value foo
...
```

Client:

```bash
$ ./kvstore-client --address 0.0.0.0:50060 --type PUT --key boo --value foo
KV client sent: PUT (boo, foo)
KV Client recv: OK
$ ./kvstore-client --address 0.0.0.0:50061 --type GET --key boo
KV client sent: GET (boo)
KV Client recv: foo
```
