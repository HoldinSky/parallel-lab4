cmake_minimum_required(VERSION 3.26)
project(lab4_solution)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory("${PROJECT_SOURCE_DIR}/server" "${PROJECT_SOURCE_DIR}/_server_output")
add_subdirectory("${PROJECT_SOURCE_DIR}/client" "${PROJECT_SOURCE_DIR}/_client_output")

add_library(resources.h resources.cpp)