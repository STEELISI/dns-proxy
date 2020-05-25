#!/usr/bin/env python3
import uuid
import random

# Generate a bind9 config with 200,000 random, valid TLD entries
# Also creates a dnsperf profile to fit it
bind9_conf = open('/etc/bind/named.conf.default-zones', 'a')
tld_list = open('tld_list.txt', 'a')

# Get list of all valid TLDs
tlds = list()
with open('tld.txt') as tld_file:
    tlds = [line.rstrip() for line in tld_file]

# Generate random UUIDs as domain and add db entry
for i in range(3, 2000003):
    url = str(uuid.uuid4().hex) + '.' + random.choice(tlds)
    print(url)

    with open('/etc/bind/db.' + url, 'w') as f:
        f.write('$TTL    604800\n')
        f.write('@       IN      SOA     ns.' + url + '. root.' + url + '. (\n')
        f.write('                  ' + str(i) + '       ; Serial\n')
        f.write('             604800     ; Refresh\n')
        f.write('              86400     ; Retry\n')
        f.write('            2419200     ; Expire\n')
        f.write('             604800 )   ; Negative Cache TTL)\n')
        f.write(';\n')
        f.write('@       IN      NS      ns.' + url + '.\n')
        f.write('ns      IN      A       10.1.1.1\n')
        f.write('box      IN      A       10.1.1.3\n')
        f.write('server      IN      A       10.1.1.2\n')

    # Add zone to bind9 config
    bind9_conf.write('zone "' + url + '" {\n')
    bind9_conf.write('        type master;\n')
    bind9_conf.write('        file "/etc/bind/db.' + url + '";\n')
    bind9_conf.write('};\n')

    # Add zone to TLD list file
    tld_list.write('box.' + url + ' A\n')
    tld_list.write('server.' + url + ' A\n')
    tld_list.write('ns.' + url + ' A\n')

tld_list.close()
bind9_conf.close()
