cmake_minimum_required(VERSION 3.16)
project(dns_proxy)

set(CMAKE_CXX_STANDARD 14)

add_executable(dns_proxy main.cpp dns.h dns.cpp)