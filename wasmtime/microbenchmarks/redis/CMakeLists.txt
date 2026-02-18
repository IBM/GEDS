include_directories("/usr/include/hiredis")
link_directories("/usr/lib/aarch64-linux-gnu")
add_executable(redis_microbenchmark src/main.cpp)
target_link_libraries(redis_microbenchmark hiredis)