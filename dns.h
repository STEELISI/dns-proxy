#ifndef DNS_PROXY__DNS_H
#define DNS_PROXY__DNS_H

#include <string>
#include <fstream>
#include <unordered_set>

// Check whether a packet is a standard QUERY
bool check_if_query(const uint8_t *buffer);

// Check if a packet is going to have an invalid TLD
bool check_if_tld_valid(const uint8_t *buffer);

// Get a domain name from a packet
std::string get_domain_name(const uint8_t *buffer);

// Read valid TLDs from file into valid_tlds
void insert_tlds(std::string input_file);

// Create an NXDOMAIN reply to the packet in the buffer
uint8_t **create_nxdomain_reply(const uint8_t *buffer);

#endif //DNS_PROXY__DNS_H
