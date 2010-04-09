#!/usr/bin/env python

import sys
import struct
from operator import itemgetter

EVENT_MALLOC  = 0x01
EVENT_FREE    = 0x02
EVENT_BT_ITEM = 0x03

UNKNOWN = '<unknown>'
KMALLOC = 'kmalloc'
EMPTY   = ''

f = open(sys.argv[1], "rb")
data = f.read()
f.close()

i = 0
last_ptr = None
alloc_table = {}
freq = {}

while i < len(data) :
    type = ord(data[i])
    i += 1

    if type == EVENT_MALLOC :
        size = struct.unpack("I",data[i:i+4])[0]
        i += 4
        ptr = struct.unpack("I",data[i:i+4])[0]
        i += 4

        if ptr in alloc_table :
            print "Memory at %x is already allocated." % ptr
            sys.exit(0)
        else :
            alloc_table[ptr] = { "size" : size, "trace" : [] }
            last_ptr = ptr
    elif type == EVENT_FREE :
        ptr = struct.unpack("I",data[i:i+4])[0]
        i += 4

        if not ptr in alloc_table :
            print "Memory at %x is not yet allocated." % ptr
        else :
            alloc_table.pop(ptr)
    elif type == EVENT_BT_ITEM :
        ip = struct.unpack("I",data[i:i+4])[0]
        i += 4
        name_length = ord(data[i])
        i += 1
        name = data[i:i+name_length]
        i += name_length
        #alloc_table[last_ptr]["trace"] += [name + "@%x" % ip]
        alloc_table[last_ptr]["trace"] += [name]
    else :
        print "Invalid event: %d, at position %d" % (type, i)
        sys.exit(1)

def sort_dict(d, reverse=False):
    ''' fast sorting method, as proposed in PEP 265, using the itemgetter '''
    return sorted(d.iteritems(), key=itemgetter(1), reverse=True)

def add_freq(item, num_bytes):
    global freq
    if item not in [UNKNOWN, KMALLOC, EMPTY]:
        if item not in freq:
            freq[item] = 0
        freq[item] += num_bytes

def print_alloc_table():
    global alloc_table

    for ptr, v in alloc_table.items():
        print "%08x %d %s" % (ptr, v["size"], ",".join(v["trace"]))
        for fn in v["trace"]:
            add_freq(fn, v["size"])

def print_cum_freqs():
    global freq

    freq_sorted = sort_dict(freq)

    for (k, v) in freq_sorted:
        print k, v

print "*"*30
print "Allocation table"
print "*"*30
print_alloc_table()
print "*"*30
print "Cumulative leaked memory"
print "*"*30
print_cum_freqs()
