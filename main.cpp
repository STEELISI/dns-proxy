#include <csignal>
#include <getopt.h>
#include <iostream>
#include <signal.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_bus_pci.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_malloc.h>
#include <rte_kni.h>

#include "dns.h"

/* Macros for printing using RTE_LOG */
#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

/* Max size of a single packet */
#define MAX_PACKET_SZ           2048

/* Size of the data buffer in each mbuf */
#define MBUF_DATA_SZ (MAX_PACKET_SZ + RTE_PKTMBUF_HEADROOM)

/* Number of mbufs in mempool that is created */
#define NB_MBUF                 (8192 * 16)

/* How many packets to attempt to read from NIC in one go */
#define PKT_BURST_SZ            32

/* How many objects (mbufs) to keep in per-lcore mempool cache */
#define MEMPOOL_CACHE_SZ        PKT_BURST_SZ

/* Number of RX ring descriptors */
#define NB_RXD                  1024

/* Number of TX ring descriptors */
#define NB_TXD                  1024

/* Total octets in ethernet header */
#define KNI_ENET_HEADER_SIZE    14

/* Total octets in the FCS */
#define KNI_ENET_FCS_SIZE       4

#define KNI_US_PER_SECOND       1000000
#define KNI_SECOND_PER_DAY      86400

#define KNI_MAX_KTHREAD 32

rte_mempool *pktmbuf_pool = nullptr;

/* Options for configuring ethernet port */
static struct rte_eth_conf port_conf = {
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};

void init_port(unsigned int port) {
  uint16_t nb_rxd = NB_RXD;
  uint16_t nb_txd = NB_TXD;

  /* Initialise device and RX/TX queues */
  RTE_LOG(INFO, APP, "Initialising port %u ...\n", (unsigned) port);
  fflush(stdout);

  struct rte_eth_dev_info dev_info;
  int ret = rte_eth_dev_info_get(port, &dev_info);
  if (ret != 0)
    rte_exit(EXIT_FAILURE,
             "Error during getting device (port %u) info: %s\n",
             port,
             strerror(-ret));

  struct rte_eth_conf local_port_conf = port_conf;
  if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
    local_port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

  ret = rte_eth_dev_configure(port, 1, 1, &local_port_conf);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not configure port%u (%d)\n", (unsigned) port, ret);

  ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
  if (ret < 0)
    rte_exit(EXIT_FAILURE,
             "Could not adjust number of descriptors for port%u (%d)\n",
             (unsigned) port,
             ret);

  struct rte_eth_rxconf rxq_conf = dev_info.default_rxconf;
  rxq_conf.offloads = local_port_conf.rxmode.offloads;
  ret =
      rte_eth_rx_queue_setup(port, 0, nb_rxd, rte_eth_dev_socket_id(port), &rxq_conf, pktmbuf_pool);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not setup up RX queue for port%u (%d)\n", (unsigned) port, ret);

  struct rte_eth_txconf txq_conf = dev_info.default_txconf;
  txq_conf.offloads = local_port_conf.txmode.offloads;
  ret = rte_eth_tx_queue_setup(port, 0, nb_txd, rte_eth_dev_socket_id(port), &txq_conf);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not setup up TX queue for port%u (%d)\n", (unsigned) port, ret);

  ret = rte_eth_dev_start(port);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not start port%u (%d)\n", (unsigned) port, ret);

  ret = rte_eth_promiscuous_enable(port);
  if (ret != 0)
    rte_exit(EXIT_FAILURE,
             "Could not enable promiscuous mode for port%u: %s\n",
             port,
             rte_strerror(-ret));
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

  // Read in input file of valid TLDs
  insert_tlds("tld.txt");

  // Initialize EAL
  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not initialise EAL (%d)\n", ret);

  // Create mubf pool
  pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", 131072,
                                         32, 0, 4096, rte_socket_id());
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

    init_port(port);
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
