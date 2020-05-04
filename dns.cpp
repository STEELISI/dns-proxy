#include "dns.h"

#include <algorithm>
#include <cstring>
#include <dpdk/rte_ether.h>
#include <dpdk/rte_ip.h>
#include <dpdk/rte_malloc.h>
#include <dpdk/rte_memcpy.h>
#include <dpdk/rte_udp.h>
#include <fstream>
#include <netinet/in.h>
#include <unordered_set>

std::unordered_set<std::string> valid_tlds;

// Local IP address, change as needed (currently 10.1.1.2)
uint32_t local_ip = 0x0201010a;

bool check_if_query(const rte_mbuf *pkt) {
  rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, rte_ether_hdr *);

  // End if not an IPv4 packet
  if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4))
    return false;

  rte_ipv4_hdr *ip_hdr = rte_pktmbuf_mtod_offset(pkt, rte_ipv4_hdr *,
                                                 sizeof(struct rte_ether_hdr));

  // Also end if destination is not us
  if (ip_hdr->dst_addr != local_ip)
    return false;

  rte_udp_hdr *udp_hdr =
      (rte_udp_hdr *)((unsigned char *)ip_hdr + sizeof(struct rte_ipv4_hdr));

  // Not to DNS port
  if (rte_be_to_cpu_16(udp_hdr->dst_port) != 53)
    return false;

  // Get the second set of 16 bits
  char *dns_hdr = (char *)udp_hdr + sizeof(rte_udp_hdr);
  char *flags = 0, *qdcount_1, *qdcount_2 = 0;
  flags = dns_hdr + 2;
  qdcount_1 = dns_hdr + 4;
  qdcount_2 = qdcount_1 + 1;

  // Return false if not standard query
  if ((*flags >> 3) != 0)
    return false;

  // Return true only if exactly one query
  return (*qdcount_1 == 0 && *qdcount_2 == 1);
}

std::string get_domain_name(const rte_mbuf *pkt) {
  // Skip headers
  rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
  rte_ipv4_hdr *ip_hdr = rte_pktmbuf_mtod_offset(pkt, rte_ipv4_hdr *,
                                                 sizeof(struct rte_ether_hdr));
  rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                                sizeof(struct rte_ipv4_hdr));
  char *qname = (char *)udp_hdr + sizeof(struct rte_udp_hdr) + 12;

  // Loop until the end of the query name
  std::string query;
  int str_len, offset = 0;
  char *qname_start = qname;

  while (true) {
    // Read in the string length
    str_len = *qname;
    qname = qname + str_len + 1;
    if (*qname != 0x0)
      offset += str_len + 1;
    else
      break;
  }
#include <dpdk/rte_ether.h>

  for (int i = offset + 1; i <= offset + str_len; i++)
    query.push_back(*(qname_start + i));

  return query;
}

int check_if_tld_valid(const rte_mbuf *pkt) {
  std::string domain = get_domain_name(pkt);

  // Get TLD and make it uppercase
  std::string tld =
      domain.substr(domain.find_last_of('.') + 1, domain.length() - 1);
  std::for_each(tld.begin(), tld.end(), [](char &c) { c = std::toupper(c); });

  // Return true if the TLD is in valid_tlds
  return valid_tlds.find(tld) != valid_tlds.end();
}

rte_mbuf *create_nxdomain_reply(const rte_mbuf *pkt) {
  rte_mbuf *ret_packet = (rte_mbuf *)rte_malloc(nullptr, sizeof(rte_mbuf), 0);
  // Set up input headers
  rte_ether_hdr *out_eth_hdr =
      (rte_ether_hdr *)rte_pktmbuf_append(ret_packet, sizeof(rte_ether_hdr));
  rte_ipv4_hdr *out_ip_hdr =
      (rte_ipv4_hdr *)rte_pktmbuf_append(ret_packet, sizeof(rte_ipv4_hdr));
  rte_udp_hdr *out_udp_hdr =
      (rte_udp_hdr *)rte_pktmbuf_append(ret_packet, sizeof(rte_udp_hdr));

  // Set up output headers
  rte_ether_hdr *in_eth_hdr = rte_pktmbuf_mtod(pkt, rte_ether_hdr *);
  rte_ipv4_hdr *in_ip_hdr =
      rte_pktmbuf_mtod_offset(pkt, rte_ipv4_hdr *, sizeof(rte_ether_hdr));
  rte_udp_hdr *in_udp_hdr =
      (struct rte_udp_hdr *)((unsigned char *)in_ip_hdr + sizeof(rte_ipv4_hdr));

  // Set up Ethernet header
  rte_memcpy(&out_eth_hdr, &in_eth_hdr, sizeof(out_eth_hdr));
  rte_memcpy(&out_eth_hdr->d_addr, &in_eth_hdr->s_addr,
             sizeof(out_eth_hdr->d_addr));
  rte_memcpy(&out_eth_hdr->s_addr, &in_eth_hdr->d_addr,
             sizeof(out_eth_hdr->s_addr));

  // Set up IPv4 header
  rte_memcpy(&out_ip_hdr, &in_ip_hdr, sizeof(out_ip_hdr));
  rte_memcpy(&out_ip_hdr->dst_addr, &in_ip_hdr->src_addr,
             sizeof(out_ip_hdr->dst_addr));
  rte_memcpy(&out_ip_hdr->src_addr, &in_ip_hdr->dst_addr,
             sizeof(out_ip_hdr->src_addr));

  // Set up UDP header
  rte_memcpy(&out_udp_hdr, &in_udp_hdr, sizeof(out_udp_hdr));
  rte_memcpy(&out_udp_hdr->dst_port, &in_udp_hdr->src_port,
             sizeof(out_udp_hdr->dst_port));
  rte_memcpy(&out_udp_hdr->src_port, &in_udp_hdr->dst_port,
             sizeof(out_udp_hdr->src_port));
  out_udp_hdr->dgram_cksum = 0; // Ignore UDP checksum

  // Set UDP and IPv4 length


  // Set IPv4 checksum
  out_ip_hdr->hdr_checksum = 0;
  out_ip_hdr->hdr_checksum = rte_ipv4_cksum(out_ip_hdr);

  return ret_packet;
}

bool tld_setup() {
  std::ifstream tld_in;
  std::string line;

  tld_in.open("tld.txt");

  while (getline(tld_in, line)) {
    // Make sure we're not getting a comment
    if (line[0] != '#')
      valid_tlds.insert(line);
  }

  return true;
}
