#include "dns.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <unordered_set>

std::unordered_set<std::string> *valid_tlds =
    new std::unordered_set<std::string>;

bool request_is_query(const uint8_t *buffer) {
  // Get the second set of 16 bits
  uint16_t first_16_bits = 0;
  memcpy(&first_16_bits, buffer + 2, 2);

  // Read byte 16 and make sure it's a query first
  if ((first_16_bits & 0x8000) != 0x0000)
    return false;

  // Read bytes 17-20 and shift back
  auto opcode = (first_16_bits & 0x7800) >> 11;

  // Return true if the request is a standard query
  return (opcode == 0x0000);
}

void insert_tlds(const std::string &input_file) {
  std::ifstream file(input_file);
  std::string in_str;
  // Insert everything but comments
  while (std::getline(file, in_str))
    if (in_str.length() > 0 && in_str[0] != '#')
      valid_tlds->insert(in_str);
}

std::string get_domain_name(const uint8_t *buffer) {
  std::string domain;
  // Skip header
  int i = 12;

  while (buffer[i] != 0) {
    domain.push_back(buffer[i]);
    i++;
  }

  return domain;
}

bool check_if_tld_valid(const uint8_t *buffer) {
  std::string domain = get_domain_name(buffer);

  // Get TLD and make it uppercase
  std::string tld =
      domain.substr(domain.find_last_of('.') + 1, domain.length() - 1);
  std::for_each(tld.begin(), tld.end(), [](char &c) { c = std::toupper(c); });

  // Return true if the TLD is in valid_tlds
  return valid_tlds->find(tld) != valid_tlds->end();
}

uint8_t **create_nxdomain_reply(const uint8_t *buffer) {
  uint8_t *ret_packet;

  return &ret_packet;
}
