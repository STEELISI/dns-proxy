# dns-proxy
a DPDK KNI proxy to catch DNS requests with invalid TLDs before they reach the DNS server

## Requirements
* [DPDK](https://dpdk.org) built with libpcap support
* CMake
* A CPU with SSSE3 support

## Compilation
I've only tested compilation with GCC 9 and ICC 2020.2. It may work on other compilers.

DPDK was at version 19.11.2 when I tested it.

Build with `cmake` by running:

```bash
mkdir build && cd build
cmake ..
make
```

## Running
Before running, remember to correctly set `local_ip` in `dns.cpp` to the IP address of your DNS server.

This program is based off DPDK's KNI example program, so EAL and command-line options will be the same. Note that CPUs `LCORE_WORKER_TX` and `LCORE_WORKER_TX` should also be assigned, and it must be run in the same directory as `tld.txt`. For example, given a build in the `build` folder and a interface on 0000:04:00.0 bound to NUMA node 0, and 4 GB of hugepage memory:

```
sudo ./build/dns_proxy -l 0,4,8,12,16 --master-lcore 0 -w 0000:04:00.0 -m 4096 -- -p 0x1 -P --config '(0,4,8,12,16)'
```
