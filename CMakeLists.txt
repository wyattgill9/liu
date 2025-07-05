cmake_minimum_required(VERSION 3.30.0)
project(liu)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/includes)

add_executable(liu src/main.cpp includes/simdjson.cpp)
