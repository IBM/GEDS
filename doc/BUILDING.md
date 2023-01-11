# Building

## CMake
Install CMake > 3.20.

- Build commands: 
  ```bash
  cmake -DCMAKE_BUILD_TYPE=Debug -S . -B $BUILD_DIR
  cmake --build $BUILD_DIR
  ```

- Test commands: 
  ```bash
  cmake --build $BUILD_DIR -t test
  ```

- Install command: 
  ```bash
  cmake --install $BUILD_DIR --prefix $INSTALL_DIR --component geds
  ```

## Docker

`build-docker.sh` builds a docker container with GRPC and a build of GEDS in `/usr/local/opt/geds`. 

## Dependencies

### MacOS

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

### Linux

Install the following dependencies:

```
apt-get install -y \
    clang \
    curl wget \
    build-essential gcc ninja-build \
    openjdk-11-jdk \
    python3.9 python3.9-dev python3-distutils
```

and a recent version (>= 3.20) of CMake:
```
CMAKE_VERSION=3.22.4
wget --quiet -O cmake.tar.gz https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
    && tar xf cmake.tar.gz  --strip-components=1 -C /usr/local/ \
    && rm cmake.tar.gz
```
