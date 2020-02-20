#include <csignal>
#include <getopt.h>
#include <iostream>
#include <signal.h>

#include <dpdk/rte_common.h>
#include <dpdk/rte_log.h>
#include <dpdk/rte_memory.h>
#include <dpdk/rte_memcpy.h>
#include <dpdk/rte_eal.h>
#include <dpdk/rte_per_lcore.h>
#include <dpdk/rte_launch.h>
#include <dpdk/rte_atomic.h>
#include <dpdk/rte_lcore.h>
#include <dpdk/rte_branch_prediction.h>
#include <dpdk/rte_interrupts.h>
#include <dpdk/rte_bus_pci.h>
#include <dpdk/rte_debug.h>
#include <dpdk/rte_ether.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_mempool.h>
#include <dpdk/rte_mbuf.h>
#include <dpdk/rte_string_fns.h>
#include <dpdk/rte_cycles.h>
#include <dpdk/rte_malloc.h>
#include <dpdk/rte_kni.h>

#include "dns.h"

void init_kni() {

}

void init_port(uint16_t port) {

}

/**
 * Handles SIGINT
 * @param signal signal number
 */
void handler(int signal) {
  std::cout << "Caught signal " << signal << std::endl;
  std::cout << "Exiting!" << std::endl;
  exit(0);
}

/**
 * Main method
 * @return error code
 */
int main(int argc, char *argv[]) {
  signal(SIGINT, handler);

  // Initialize EAL
  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);

  // Create mubf pool
  rte_mempool *pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", 131072,
      32, 0, 4096, rte_socket_id());
  if (pktmbuf_pool == nullptr)
    rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");

  // Make sure there's at least one port
  if (rte_eth_dev_count_avail() == 0)
    rte_exit(EXIT_FAILURE, "No supported Ethernet device found\n");

  // Init KNI
  init_kni();

  // Make sure each port is valid and then init port
  uint16_t port;
  RTE_ETH_FOREACH_DEV(port) {
    if (!rte_eth_dev_is_valid_port(port))
      rte_exit(EXIT_FAILURE, "Configured invalid port ID %u\n", port);

    init_port(port);
  }

  return 0;
}
