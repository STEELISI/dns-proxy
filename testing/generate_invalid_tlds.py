#!/usr/bin/env python3
import uuid

# Generate 600,000 invalid TLDs list for dnsperf
with open('invalid.txt', 'w') as f:
    for i in range(0, 600000):
        url = str(uuid.uuid4().hex)
        f.write(url + ' A\n')
