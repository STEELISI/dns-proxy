#include <csignal>
#include <getopt.h>
#include <iostream>
#include <signal.h>

#include "dns.h"
#include "dpdk_addons.h"

rte_mempool *pktmbuf_pool = nullptr;

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

  // Read in input file of valid TLDs
  insert_tlds("tld.txt");

  // Initialize EAL
  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);

  // Create mubf pool
  pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", 131072, 32, 0, 4096,
                                         rte_socket_id());
  if (pktmbuf_pool == nullptr)
    rte_exit(EXIT_FAILURE, "Could not initialise mbuf pool\n");

  // Make sure there's at least one port
  if (rte_eth_dev_count_avail() == 0)
    rte_exit(EXIT_FAILURE, "No supported Ethernet device found\n");

  // Init KNI with 1 interface
  rte_kni_init(1);

  // Make sure each port is valid and then init port
  unsigned int port;
  RTE_ETH_FOREACH_DEV(port) {
    if (!rte_eth_dev_is_valid_port(port))
      rte_exit(EXIT_FAILURE, "Configured invalid port ID %u\n", port);

    init_port(port, pktmbuf_pool);
  }

  while (true) {
    uint64_t freq = rte_get_timer_hz();
    uint64_t t = rte_rdtsc() + freq;
    while (rte_rdtsc() < t) {
      rte_pause();
    }
  }

  return 0;
}
