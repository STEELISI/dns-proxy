#ifndef DNS_PROXY__DNS_H
#define DNS_PROXY__DNS_H

#include <string>
#include <rte_mbuf.h>

/**
 * Check whether a packet is a standard QUERY
 * @param buffer the input packet as a byte array
 * @return true if the request is a query, false otherwise
 */
bool check_if_query(const rte_mbuf *pkt);


/**
 * Check if a packet is has an invalid TLD
 * @param buffer the input packet as a byte array
 * @return true if the TLD is valid, false otherwise
 */
bool check_if_tld_valid(const rte_mbuf *pkt);

/**
 * Puts all values from `tld.txt` into the valid_tlds unordered_set
 * @return true if success
 */
bool tld_setup();

#endif //DNS_PROXY__DNS_H
