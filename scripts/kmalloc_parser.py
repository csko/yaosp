import sys
import struct

EVENT_MALLOC  = 0x01
EVENT_FREE    = 0x02
EVENT_BT_ITEM = 0x03

f = open(sys.argv[1], "rb")
data = f.read()
f.close()

i = 0
last_ptr = None
alloc_table = {}

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
        print "Invalid event: %d" % type
        sys.exit(0)

print "Allocation table"

for ptr in alloc_table :
    print "%08x %d [%s]" % (ptr, alloc_table[ptr]["size"], ",".join(alloc_table[ptr]["trace"]))

