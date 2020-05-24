#!/usr/bin/env python3
import uuid

# Generate 10,000,000 invalid TLDs list for dnsperf
with open('invalid.txt') as f:
    for i in range(0, 10000000):
        url = str(uuid.uuid4().hex)
        f.write(url + ' A\n')
