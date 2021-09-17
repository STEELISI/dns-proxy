#include "packet_builder.h"

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

void create_nxdomain_reply(const rte_mbuf *pkt) {
  // Set up headers and data
  rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, rte_ether_hdr *);
  rte_ipv4_hdr *ip_hdr =
      rte_pktmbuf_mtod_offset(pkt, rte_ipv4_hdr *, sizeof(rte_ether_hdr));
  rte_udp_hdr *udp_hdr =
      (struct rte_udp_hdr *)((unsigned char *)ip_hdr + sizeof(rte_ipv4_hdr));
  char *dns_hdr = (char *)udp_hdr + sizeof(struct rte_udp_hdr);

  // Set up Ethernet header
  auto temp_d_addr = eth_hdr->d_addr;
  eth_hdr->d_addr = eth_hdr->s_addr;
  eth_hdr->s_addr = temp_d_addr;

  // Set up IPv4 header
  auto temp_dst_addr = ip_hdr->dst_addr;
  ip_hdr->dst_addr = ip_hdr->src_addr;
  ip_hdr->src_addr = temp_dst_addr;

  // Set up UDP header
  auto temp_dst_port = udp_hdr->dst_port;
  udp_hdr->dst_port = udp_hdr->src_port;
  udp_hdr->src_port = temp_dst_port;
  udp_hdr->dgram_cksum = 0; // Ignore UDP checksum

  // Modify DNS headers
  *(dns_hdr + 2) |= 0b10000000;   // Standard query authoritative answer, no
                                  // truncation or recursion
  *(dns_hdr + 3) = 0b00000011;    // Name error
  // Keep original question
  *(dns_hdr + 6) = 0x00;          // No answers
  *(dns_hdr + 7) = 0x00;
  *(dns_hdr + 8) = 0x00;          // 1 name server record
  *(dns_hdr + 9) = 0b00000001;    // This is big endian
  *(dns_hdr + 10) = 0x00;         // No resource records
  *(dns_hdr + 11) = 0x00;

  // Set IPv4 checksum
  ip_hdr->hdr_checksum = 0;
  ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);
}
