add_executable(sqlite_microbenchmark src/main.cpp)
set(CMAKE_CXX_FLAGS "-fno-exceptions")      # Not suported by wasi-sdk
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sqlite_microbenchmark ${CMAKE_CURRENT_SOURCE_DIR}/lib/wasm32-wasi/libsqlite3.a)