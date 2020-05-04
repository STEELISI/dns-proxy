#include "dns.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
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

int check_if_tld_valid(const rte_mbuf *pkt) {
  std::string domain = get_domain_name(pkt);

  // Get TLD and make it uppercase
  std::string tld =
      domain.substr(domain.find_last_of('.') + 1, domain.length() - 1);
  std::for_each(tld.begin(), tld.end(), [](char &c) { c = std::toupper(c); });

  // Return true if the TLD is in valid_tlds
  return valid_tlds.find(tld) != valid_tlds.end();
}

unsigned char **create_nxdomain_reply(const unsigned char *buffer) {
  unsigned char *ret_packet;

  return &ret_packet;
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
