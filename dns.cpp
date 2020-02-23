#include "dns.h"

#include <cstring>

std::unordered_set<std::string> *valid_tlds = new std::unordered_set<std::string>;

bool request_is_query(const uint8_t *buffer) {
  // Get the second set of 16 bits
  uint16_t first_16_bits = 0;
  memcpy(&first_16_bits, buffer + 2, 2);

  // Read byte 16 and make sure it's a query first
  if ((first_16_bits & 0x8000) != 0x0000)
    return false;

  // Read bytes 17-20 and shift back
  auto opcode = (first_16_bits & 0x7800) >> 11;

  // Return if the request is a standard query
  return (opcode == 0x0000);
}

void insert_tlds(std::string input_file) {

}

std::string get_domain_name(const uint8_t *buffer) {
  return "todo";
}

bool check_if_tld_valid(const uint8_t *buffer) {
  std::string domain = get_domain_name(buffer);

  // Get TLD
  std::string tld = domain.substr(domain.find_last_of('.') + 1, domain.length() - 1);

  // Return true if the TLD is in valid_tlds
  return valid_tlds->find(tld) != valid_tlds->end();
}

uint8_t **create_nxdomain_reply(const uint8_t *buffer) {
  uint8_t *ret_packet;

  return &ret_packet;
}