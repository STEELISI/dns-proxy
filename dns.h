#ifndef DNS_PROXY__DNS_H
#define DNS_PROXY__DNS_H

#include <string>

// Useful enums (taken from `github.com/ldeng-ustc/netalgo-lab`)
/**
 * Response Type
 */
enum {
  Response_OK = 0,
  Response_FormatError = 1,
  Response_ServerFailure = 2,
  Response_NameError = 3,
  Response_NotImplemented = 4,
  Response_Refused = 5
};

/**
 * Resource Record Types
 */
enum {
  Resource_A = 1,
  Resource_NS = 2,
  Resource_CNAME = 5,
  Resource_SOA = 6,
  Resource_PTR = 12,
  Resource_MX = 15,
  Resource_TXT = 16,
  Resource_AAAA = 28,
  Resource_SRV = 33
};

/**
 * OPCODEs
 */
enum {
  OPCODE_QUERY = 0, /* standard query */
  OPCODE_IQUERY = 1, /* inverse query */
  OPCODE_STATUS = 2, /* server status request */
  OPCODE_NOTIFY = 4, /* request zone transfer */
  OPCODE_UPDATE = 5 /* change resource records */
};

/**
 * RCODEs
 */
enum {
  RCODE_NOERROR = 0,
  RCODE_FORMERR = 1,
  RCODE_SERVFAIL = 2,
  RCODE_NXDOMAIN = 3
};

/* Query Type */
enum {
  Query_IXFR = 251,
  Query_AXFR = 252,
  Query_MAILB = 253,
  Query_MAILA = 254,
  Query_STAR = 255
};

struct Question {
  std::string name;
  char type;
  char class_code;
};

#endif //DNS_PROXY__DNS_H
