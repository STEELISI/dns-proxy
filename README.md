# dns-proxy
a DPDK dns proxy to catch bad NXDOMAIN requests

## Requirements
* [DPDK](https://dpdk.org) built with libpcap support
* CMake
* A CPU with SSSE3 support

## Compilation
I've only tested compilation with GCC 9 and Intel C++ Compiler.

Build with `cmake` by running:

```bash
mkdir build && cd build
cmake ..
make
```

## Running
This program is based off DPDK's KNI example program, so EAL and command-line options will be the same. Note that CPUs `LCORE_WORKER_TX` and `LCORE_WORKER_TX` should also be assigned, and it must be run in the same directory as `tld.txt`.

