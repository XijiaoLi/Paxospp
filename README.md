# Paxos-Cpp

### Build and locally install gRPC and Protocol Buffers

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

- Linux

  ```sh
  $ sudo apt install -y cmake
  ```

- macOS:

  ```sh
  $ brew install cmake
  ```

- For general `cmake` installation instructions, see [Installing CMake](https://cmake.org/install).

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

- Linux

  ```sh
  $ sudo apt install -y build-essential autoconf libtool pkg-config
  ```

- macOS:

  ```sh
  $ brew install autoconf automake libtool pkg-config
  ```

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

#### Important

We **strongly** encourage you to install gRPC *locally* — using an appropriately set `CMAKE_INSTALL_PREFIX` — because there is no easy way to uninstall gRPC after you’ve installed it globally.

### Build and run Paxos server and client

Clone this `Paxos-Cpp` repo:

```sh
$ https://github.com/XijiaoLi/Paxos-Cpp.git
```

Under `Paxos-Cpp` folder, run the following commands to build the excutable using `cmake`:

```sh
$ cd Paxos-Cpp
$ mkdir -p cmake/build
$ pushd cmake/build
$ cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
$ make -j
```

Now you should be at **build** directory `Paxos-Cpp/cmake/build`, with all the excutables.

1. Run the server:

   ```sh
   $ ./paxos
   Server listening on 0.0.0.0:50051
   ```

2. From a different terminal window, run the client and see the client output:

   ```sh
   $ ./paxos_client
   Response received: seq = 1; value = hello
   ```
