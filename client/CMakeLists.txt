cmake_minimum_required(VERSION 3.26)
project(client)

set(CMAKE_CXX_STANDARD 17)

link_directories(./**)

add_executable(client src/main.cpp
        src/communication/base.h
        src/communication/base.cpp
        ../resources.h
        ../resources.cpp
)
