# dns-proxy
a DPDK dns proxy to catch bad NXDOMAIN requests

## Installation
First, we need to install DPDK. We'll need `libnuma-dev` and `libpcap-dev` for it.

We're sticking with the 19.11.1 version for now:
```
wget http://fast.dpdk.org/rel/dpdk-19.11.1.tar.xz
tar xvf dpdk-19.11.1.tar.xz && cd dpdk-stable-19.11.1

# Enable libpcap
make config T=x86_64-native-linuxapp-gcc
sed -ri 's,(PMD_PCAP=).*,\1y,' build/.config

# Build it and install to /usr/local/
make
make install

# Load kernel modules
cd build/
insmod kmod/igb_uio.ko
insmod kmod/rte_kni.ko carrier=on

# Set up hugepages (We're doing 1GB of them)
mkdir -p /mnt/huge
mount -t hugetlbfs nodev /mnt/huge
echo 512 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
```
