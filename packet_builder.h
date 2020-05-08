#ifndef DNS_PROXY__PACKET_BUILDER_H
#define DNS_PROXY__PACKET_BUILDER_H

#include <string>
#include <dpdk/rte_mbuf.h>

/**
 * Modfiy a packet to be an NXDOMAIN reply
 * @param buffer the input packet as a byte array
 */
void create_nxdomain_reply(const rte_mbuf *pkt);

#endif //DNS_PROXY__PACKET_BUILDER_H
