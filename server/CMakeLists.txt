cmake_minimum_required(VERSION 3.26)
project(server)

set(CMAKE_CXX_STANDARD 17)

set(THREAD_POOL
        src/pool/helper.h
        src/pool/thread_pool.h
        src/pool/thread_pool.cpp)

link_directories(./**)

add_executable(server src/main.cpp
        src/algorithm.h
        src/algorithm.cpp
        src/communication/server.h
        src/communication/server.cpp
        ../resources.h
        ../resources.cpp
)
