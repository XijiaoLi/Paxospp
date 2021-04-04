# Code Directory Introduction

**lib:** This directory includes  all head files and all implementation of Paxospp, You can figure out the working principle of Paoxspp by reading this directorys. No neccessary to read it if you are only using Paxospp.

**protos**: This directory includes all proto files required by gRPC communication.

**test:** This directory includes all the test codes and test datasets.

**examples:** This directory provides an example usage of Paoxspp.

# How to Compile

## Install Third Party Libs
### CMake, gRPC and Protocol Buffers

The steps in the section explain now to build and locally install gRPC and Protocol Buffers using `cmake`. If you’d rather use [bazel](https://www.bazel.build/), see [Building from source](https://github.com/grpc/grpc/blob/master/BUILDING.md#build-from-source).

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

## Build and run Paxos server

Clone this `Paxos-Cpp` repo:

```sh
$ https://github.com/XijiaoLi/Paxos-Cpp.git
$ cd Paxos-Cpp
```

Go to the `Paxos-Cpp/example/` folder, and run the following commands to build the excutable using `cmake`:

```sh
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
$ make -j
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

# How to Wrap Your Own Code Around Paxospp
???