#include "dpdk_addons.h"

void init_port(unsigned int port, rte_mempool *pktmbuf_pool) {
  uint16_t nb_rxd = NB_RXD;
  uint16_t nb_txd = NB_TXD;

  // Initialise device and RX/TX queues
  RTE_LOG(INFO, APP, "Initialising port %u ...\n", (unsigned) port);
  fflush(stdout);

  struct rte_eth_dev_info dev_info;
  int ret = rte_eth_dev_info_get(port, &dev_info);
  if (ret != 0)
    rte_exit(EXIT_FAILURE, "Error during getting device (port %u) info: %s\n", port,
             strerror(-ret));

  struct rte_eth_conf local_port_conf = port_conf;
  if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
    local_port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

  ret = rte_eth_dev_configure(port, 1, 1, &local_port_conf);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not configure port%u (%d)\n", (unsigned) port, ret);

  ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not adjust number of descriptors for port%u (%d)\n",
             (unsigned) port, ret);

  struct rte_eth_rxconf rxq_conf = dev_info.default_rxconf;
  rxq_conf.offloads = local_port_conf.rxmode.offloads;
  ret =
      rte_eth_rx_queue_setup(port, 0, nb_rxd, rte_eth_dev_socket_id(port), &rxq_conf,
                             pktmbuf_pool);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not setup up RX queue for port%u (%d)\n", (unsigned) port,
             ret);

  struct rte_eth_txconf txq_conf = dev_info.default_txconf;
  txq_conf.offloads = local_port_conf.txmode.offloads;
  ret = rte_eth_tx_queue_setup(port, 0, nb_txd, rte_eth_dev_socket_id(port), &txq_conf);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not setup up TX queue for port%u (%d)\n", (unsigned) port,
             ret);

  // Start the port
  ret = rte_eth_dev_start(port);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not start port%u (%d)\n", (unsigned) port, ret);
}

void kni_egress(uint16_t port_id, rte_kni *kni_port)
{
  unsigned nb_tx, num;
  uint32_t nb_kni;
  struct rte_mbuf *pkts_burst[PKT_BURST_SZ];

  // Burst rx from kni
  num = rte_kni_rx_burst(kni_port, pkts_burst, PKT_BURST_SZ);
  if (unlikely(num > PKT_BURST_SZ)) {
    RTE_LOG(ERR, APP, "Error receiving from KNI\n");
    return;
  }

  // Burst tx to eth
  nb_tx = rte_eth_tx_burst(port_id, 0, pkts_burst, (uint16_t)num);

  // Free mbufs not transferred to NIC
  if (unlikely(nb_tx < num))
    for(unsigned pkt_it = 0; pkt_it < num - nb_tx; pkt_it++)
      rte_pktmbuf_free(pkts_burst[pkt_it]);
}