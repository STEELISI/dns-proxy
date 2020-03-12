#include "dpdk_addons.h"

void init_port(unsigned int port, rte_mempool *pktmbuf_pool) {
  uint16_t nb_rxd = NB_RXD;
  uint16_t nb_txd = NB_TXD;

  // Initialise device and RX/TX queues
  RTE_LOG(INFO, APP, "Initialising port %u ...\n", (unsigned)port);
  fflush(stdout);

  struct rte_eth_dev_info dev_info;
  int ret = rte_eth_dev_info_get(port, &dev_info);
  if (ret != 0)
    rte_exit(EXIT_FAILURE, "Error during getting device (port %u) info: %s\n",
             port, strerror(-ret));

  struct rte_eth_conf local_port_conf = port_conf;
  if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
    local_port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

  ret = rte_eth_dev_configure(port, 1, 1, &local_port_conf);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not configure port%u (%d)\n", (unsigned)port,
             ret);

  ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
  if (ret < 0)
    rte_exit(EXIT_FAILURE,
             "Could not adjust number of descriptors for port%u (%d)\n",
             (unsigned)port, ret);

  struct rte_eth_rxconf rxq_conf = dev_info.default_rxconf;
  rxq_conf.offloads = local_port_conf.rxmode.offloads;
  ret = rte_eth_rx_queue_setup(port, 0, nb_rxd, rte_eth_dev_socket_id(port),
                               &rxq_conf, pktmbuf_pool);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not setup up RX queue for port%u (%d)\n",
             (unsigned)port, ret);

  struct rte_eth_txconf txq_conf = dev_info.default_txconf;
  txq_conf.offloads = local_port_conf.txmode.offloads;
  ret = rte_eth_tx_queue_setup(port, 0, nb_txd, rte_eth_dev_socket_id(port),
                               &txq_conf);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not setup up TX queue for port%u (%d)\n",
             (unsigned)port, ret);

  // Start the port
  ret = rte_eth_dev_start(port);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Could not start port%u (%d)\n", (unsigned)port,
             ret);
}

unsigned int kni_change_mtu(uint16_t port, unsigned int new_mtu,
                            rte_mempool *pktmbuf_pool) {
  uint16_t nb_rxd = 1024;
  rte_eth_conf conf;
  rte_eth_dev_info dev_info;
  rte_eth_rxconf rxq_conf;

  // Stop the port
  rte_eth_dev_stop(port);
  memcpy(&conf, &port_conf, sizeof(conf));

  // Set new MTU
  if (new_mtu > RTE_ETHER_MAX_LEN)
    conf.rxmode.offloads |= DEV_RX_OFFLOAD_JUMBO_FRAME;
  else
    conf.rxmode.offloads &= ~DEV_RX_OFFLOAD_JUMBO_FRAME;

  // Set up max packet length including headers
  conf.rxmode.max_rx_pkt_len =
      new_mtu + KNI_ENET_HEADER_SIZE + KNI_ENET_FCS_SIZE;

  // Reconfigure port and increase descriptors to 1024
  rte_eth_dev_configure(port, 1, 1, &conf);
  rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, nullptr);

  // Setup RX queue
  rxq_conf = dev_info.default_rxconf;
  rxq_conf.offloads = conf.rxmode.offloads;
  rte_eth_rx_queue_setup(port, 0, nb_rxd, rte_eth_dev_socket_id(port),
                         &rxq_conf, pktmbuf_pool);

  // Start the port
  rte_eth_dev_start(port);
}

rte_kni *init_kni(unsigned int port, uint8_t mac_addr[],
                  rte_mempool *pktmbuf_pool) {
  rte_kni_conf conf;
  rte_kni_ops ops;
  rte_eth_dev_info dev_info;

  // Initialize the KNI
  rte_kni_init(1);

  // Clear conf and ops
  memset(&conf, 0, sizeof(conf));
  memset(&ops, 0, sizeof(ops));

  rte_eth_dev_get_mtu(port, &conf.mtu);

  conf.min_mtu = dev_info.min_mtu;
  conf.max_mtu = dev_info.max_mtu;

  // Set stuff up
  ops.port_id = port;
  ops.change_mtu = reinterpret_cast<int (*)(uint16_t, unsigned int)>(
      kni_change_mtu(port, 1500, pktmbuf_pool));
  ops.config_network_if =
      reinterpret_cast<int (*)(uint16_t, uint8_t)>(rte_eth_dev_start(port));
  ops.config_mac_address = reinterpret_cast<int (*)(uint16_t, uint8_t *)>(
      rte_eth_dev_default_mac_addr_set(port,
                                       (struct rte_ether_addr *)mac_addr));
  return rte_kni_alloc(pktmbuf_pool, &conf, &ops);
}

void kni_egress(uint16_t port_id, rte_kni *kni_port) {
  unsigned nb_tx, num;
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
    for (unsigned pkt_it = 0; pkt_it < num - nb_tx; pkt_it++)
      rte_pktmbuf_free(pkts_burst[pkt_it]);
}
