# Building GEDS

- [Workflow](#workflow)
- [Instructions for MacOS](#instructions-for-macos)
- [Instructions for Windows](#instructions-for-windows)
- [Instructions for Linux](#instructions-for-linux)
- [Deploying via Docker](#deploying-via-docker)
- [Deploying via Ansible](#deploying-via-ansible)

## Workflow <a name="workflow"></a>
The general workflow of building GEDS from source is:
1. Pull GEDS repository: `git pull https://github.com/IBM/GEDS.git`
2. Install dependencies, e.g. `cmake` version > 3.20 (check via `cmake --version`)
3. Create `build` and `install` directory in the GEDS folder and set environment variables: `export $BUILD_DIR=~/GEDS/build` & `export $INSTALL_DIR=~/GEDS/bin`
4. Build Boost
5. Build AWS SDK
6. Build GEDS
7. Install GEDS

## Instructions for MacOS <a name="instructions-for-macos"></a>

Install the following dependencies through homebrew:

```bash
brew install cmake openjdk@11 python3
```
Note: `cmake` is not required for `CLion`.

Set the following environment variables:
```bash
export JAVA_HOME=$(brew --prefix openjdk@11)/libexec/openjdk.jdk/Contents/Home/
export OPENSSL_ROOT_DIR="$(brew --prefix openssl@1.1)"
```

Build and install the AWS SDK release targeting the desired build type:
```bash
BUILD_TYPE=Debug ./build-aws-sdk.sh
```

Configure GEDS with (and make sure to specify the build type)
```bash
cmake path/to/GEDS -DCMAKE_BUILD_TYPE=Debug
```
Finally build it with:
```bash
cmake --build . --target all
```

## Instructions for Windows <a name="instructions-for-windows"></a>
Coming

## Instructions for Linux <a name="instructions-for-linux"></a>
Install GEDS dependencies:

```
sudo apt install -y clang curl wget build-essential gcc ninja-build openjdk-11-jdk python3-dev python3-distutils cmake
```

CMake version >= 3.20:
```
CMAKE_VERSION=3.22.4
wget --quiet -O cmake.tar.gz https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
    && tar xf cmake.tar.gz  --strip-components=1 -C /usr/local/ \
    && rm cmake.tar.gz
```

Install AWS SDK dependecies:
```
sudo apt install libcurl4-openssl-dev libssl-de uuid-dev zlib1g-dev libpulse-dev
```

Build AWS SDK: `/bin/bash build-aws-sdk.sh`

Build Boost: `/bin/bash build-boost.sh`

Build GEDS:
1. Check if environment variables are correctly set via `printenv | grep BUILD_DIR` and `printenv | grep INSTALL_DIR`
2. `cmake -DCMAKE_BUILD_TYPE=Debug -S . -B $BUILD_DIR`
3. `cmake --build $BUILD_DIR -j 4` (-j specifies the number of cores to use)
4. `cmake --install $BUILD_DIR --prefix $INSTALL_DIR --component geds`

## Deploying via Docker  <a name="deploying-via-docker"></a>
`build-docker.sh` builds a docker container with GRPC and a build of GEDS in `/usr/local/opt/geds`. 

## Deploying via Ansible  <a name="deploying-via-ansible"></a>
We offer an Ansible playbook to automate GEDS building from source on multiple clients.
