cmake_minimum_required(VERSION 3.16)
project(dns_proxy)

SET(CMAKE_C_FLAGS -mssse3)
SET(CMAKE_CXX_FLAGS -mssse3)
set(LIBRARIES ${LIBRARIES} rt pthread -Wl,--whole-archive dpdk -Wl,--no-whole-archive numa dl pcap)
find_path(DPDK_INCLUDE_DIR NAMES rte_ethdev.h PATH_SUFFIXES dpdk)
include_directories(SYSTEM ${DPDK_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 14)

add_library(dns_funcs dns.h dns.cpp)
add_library(pkt_builder packet_builder.h packet_builder.cpp)

add_executable(dns_proxy main.cpp)
target_link_libraries(dns_proxy dns_funcs pkt_builder ${LIBRARIES})
