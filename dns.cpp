#include "dns.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <netinet/in.h>
#include <unordered_set>

std::unordered_set<std::string> valid_tlds;

// Local IP address, change as needed (currently 10.1.1.2)
uint32_t local_ip = 0x0201010a;

bool check_if_query(rte_mbuf *pkt) {
  struct rte_ether_hdr *eth_hdr;
  struct rte_ipv4_hdr *ip_hdr;
  struct rte_udp_hdr *udp_hdr;

  eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

  // End if not an IPv4 packet
  if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4))
    return false;

  ip_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *,
                                   sizeof(struct rte_ether_hdr));

  // Also end if destination is not us
  if (ip_hdr->dst_addr != local_ip)
    return false;

  udp_hdr = (struct rte_udp_hdr *) ((unsigned char *) ip_hdr + sizeof(struct rte_ipv4_hdr));

  // Not to DNS port
  if(rte_be_to_cpu_16(udp_hdr->dst_port) != 53)
    return false;

  // Get the second set of 16 bits
  char* buffer = (char *) udp_hdr + sizeof(struct rte_udp_hdr) + 2;
  uint16_t first_16_bits = 0;
  memcpy(&first_16_bits, buffer + 2, 2);

  // Read byte 16 and make sure it's a query first
  if ((first_16_bits >> 15) != 0x0000)
    return false;

  // Return true if standard query
  if ((first_16_bits >> 11) ==  0)
    return true;
}

std::string get_domain_name(const unsigned char *buffer) {
  std::string domain;
  // Skip header
  int i = 12;

  while (buffer[i] != 0) {
    domain.push_back(buffer[i]);
    i++;
  }

  return domain;
}

int check_if_tld_valid(const unsigned char *buffer) {
  std::string domain = get_domain_name(buffer);

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
