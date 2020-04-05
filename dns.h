#ifndef DNS_PROXY__DNS_H
#define DNS_PROXY__DNS_H

#include <string>
/**
 * Check whether a packet is a standard QUERY
 * @param buffer the input packet as a byte array
 * @return 1 if the request is a query, 0 otherwise
 */
extern "C" int check_if_query(const uint8_t *buffer);


/**
 * Check if a packet is has an invalid TLD
 * @param buffer the input packet as a byte array
 * @return 1 if the TLD is valid, 0 otherwise
 */
extern "C" int check_if_tld_valid(const uint8_t *buffer);

/**
 * Get the domain name from a packet
 * @param buffer the input packet as a byte array
 * @return the domain name as a cstring
 */
extern "C" char* get_domain_name(const uint8_t *buffer);

/**
 * Create an NXDOMAIN reply to the packet in the buffer
 * @param buffer the input packet as a byte array
 * @return a packet as a byte array
 */
extern "C" uint8_t **create_nxdomain_reply(const uint8_t *buffer);

/**
 * Read each line of a input file for TLDs
 * @param input_file location of input tld
 */
extern "C" void insert_tlds(const std::string& input_file);

#endif //DNS_PROXY__DNS_H
