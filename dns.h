#ifndef DNS_PROXY__DNS_H
#define DNS_PROXY__DNS_H

#include <string>

// Useful enums (taken from `github.com/ldeng-ustc/netalgo-lab`)
/**
 * Response Type
 */
enum {
  Ok_ResponseType = 0,
  FormatError_ResponseType = 1,
  ServerFailure_ResponseType = 2,
  NameError_ResponseType = 3,
  NotImplemented_ResponseType = 4,
  Refused_ResponseType = 5
};

/**
 * Resource Record Types
 */
enum {
  A_Resource_RecordType = 1,
  NS_Resource_RecordType = 2,
  CNAME_Resource_RecordType = 5,
  SOA_Resource_RecordType = 6,
  PTR_Resource_RecordType = 12,
  MX_Resource_RecordType = 15,
  TXT_Resource_RecordType = 16,
  AAAA_Resource_RecordType = 28,
  SRV_Resource_RecordType = 33
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
  IXFR_QueryType = 251,
  AXFR_QueryType = 252,
  MAILB_QueryType = 253,
  MAILA_QueryType = 254,
  STAR_QueryType = 255
};

struct Question {
  std::string name;
  char type;
  char class_code;
};

#endif //DNS_PROXY__DNS_H
