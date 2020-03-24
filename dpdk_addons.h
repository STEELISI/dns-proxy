#ifndef DNS_PROXY__DPDK_ADDONS_H
#define DNS_PROXY__DPDK_ADDONS_H

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

// Number of TX ring descriptors
#define NB_TXD                  1024

// Total octets in ethernet header
#define KNI_ENET_HEADER_SIZE    14

// Total octets in the FCS
#define KNI_ENET_FCS_SIZE       4

#define KNI_US_PER_SECOND       1000000
#define KNI_SECOND_PER_DAY      86400

#define KNI_MAX_KTHREAD 32

// Options for configuring ethernet port
extern "C" {
    static struct rte_eth_conf port_conf = {
        .txmode = {
            .mq_mode = ETH_MQ_TX_NONE,
        },
    };
}

/**
 * Initialize a physical DPDK port
 * @param port the port number to init
 * @param pktmbuf_pool a pointer to the global mempool
 */
void init_port(unsigned int port, rte_mempool *pktmbuf_pool);

/**
 * Change the MTU of a KNI port
 * @param port port number to change
 * @param new_mtu new mtu to set
 * @return success
 */
unsigned int kni_change_mtu(uint16_t port, unsigned int new_mtu, rte_mempool *pktmbuf_pool);

/**
 * Initialize a KNI port
 * @param port port number to bind to
 * @param mac_addr mac address to give to port
 * @param pktmbuf_pool a pointer to the global mempool
 * @return a KNI context pointer
 */
rte_kni *init_kni(unsigned int port, uint8_t mac_addr[], rte_mempool *pktmbuf_pool);

/**
 * Transmit data from the KNI port to the NIC
 * @param port_id the ID of the NIC to transmit to
 * @param kni_port a KNI context pointer to the port to transfer from
 */
void kni_egress(uint16_t port_id, rte_kni *kni_port);

#endif