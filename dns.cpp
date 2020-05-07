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

  for (int i = offset + 1; i <= offset + str_len; i++)
    query.push_back(*(qname_start + i));

  return query;
}

bool check_if_tld_valid(const rte_mbuf *pkt) {
  std::string domain = get_domain_name(pkt);

  // Get TLD and make it uppercase
  std::string tld =
      domain.substr(domain.find_last_of('.') + 1, domain.length() - 1);
  std::for_each(tld.begin(), tld.end(), [](char &c) { c = std::toupper(c); });

  // Return true if the TLD is in valid_tlds
  return valid_tlds.find(tld) != valid_tlds.end();
}

int get_name_length(const rte_mbuf *pkt) {
  // Skip headers
  rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
  rte_ipv4_hdr *ip_hdr = rte_pktmbuf_mtod_offset(pkt, rte_ipv4_hdr *,
                                                 sizeof(struct rte_ether_hdr));
  rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((unsigned char *)ip_hdr +
                                                sizeof(struct rte_ipv4_hdr));
  char *qname = (char *)udp_hdr + sizeof(struct rte_udp_hdr) + 12;

  int str_len = 0;

  while (true) {
    str_len++;
    if (*qname == 0x0)
      break;
    qname++;
  }
  return str_len;
}

void create_nxdomain_reply(const rte_mbuf *pkt) {
  // Set up headers and data
  rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, rte_ether_hdr *);
  rte_ipv4_hdr *ip_hdr =
      rte_pktmbuf_mtod_offset(pkt, rte_ipv4_hdr *, sizeof(rte_ether_hdr));
  rte_udp_hdr *udp_hdr =
      (struct rte_udp_hdr *)((unsigned char *)ip_hdr + sizeof(rte_ipv4_hdr));
  char *query = (char *)udp_hdr + sizeof(struct rte_udp_hdr);

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

  // Modify DNS header
  char *dns_hdr = query + 2;     // Copy over transaction ID
  *(dns_hdr + 2) = 0b10000100;   // Standard query authoritative answer, no
                                 // truncation or recursion
  *(dns_hdr + 3) = 0b00000011;   // Name error
  *(dns_hdr + 4) = 0x00;         // Original question
  *(dns_hdr + 5) = 0b00000001;   // This is big endian
  *(dns_hdr + 6) = 0x00;         // No answers
  *(dns_hdr + 7) = 0x00;
  *(dns_hdr + 8) = 0x00;         // No name server records
  *(dns_hdr + 9) = 0x00;
  *(dns_hdr + 10) = 0x00;        // No resource records
  *(dns_hdr + 11) = 0x00;

  // Set IPv4 checksum
  ip_hdr->hdr_checksum = 0;
  ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);
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
