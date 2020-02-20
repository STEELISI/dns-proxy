cmake_minimum_required(VERSION 3.16)
project(dns_proxy)

set(LIBRARIES ${LIBRARIES} rt pthread -Wl,--whole-archive dpdk -Wl,--no-whole-archive numa dl)
find_path(DPDK_INCLUDE_DIR NAMES rte_ethdev.h PATH_SUFFIXES dpdk)
include_directories(SYSTEM ${DPDK_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 14)

add_executable(dns_proxy main.cpp dns.h dns.cpp)
target_link_libraries(dns_proxy ${LIBRARIES})