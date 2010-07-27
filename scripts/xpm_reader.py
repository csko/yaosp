import sys

class Color :
    def __init__(self, c) :
        self.r = int("0x%s" % c[0:2], 16)
        self.g = int("0x%s" % c[2:4], 16)
        self.b = int("0x%s" % c[4:6], 16)

    def toColor(self) :
        return "yguipp::Color(%d, %d, %d)" % (self.r, self.g, self.b)

def extractHeaderData(header) :
    s = header.split(" ")
    return ( int(s[0]), int(s[1]),
             int(s[2]), int(s[3]) )

if len(sys.argv) < 2 :
    sys.exit(1)

# Read the whole XPM data from the file

f = open(sys.argv[1], "r")
data = f.readlines()
f.close()

# Convert the data ...

lines = []
for l in data :
    while len(l) > 0 and l[-1] in ["\n", ",", ";", "}"] :
        l = l[:-1]

    if l[0] != '"' or l[-1] != '"' :
        continue

    lines += [ l[1:-1] ]

(w, h, cc, t) = extractHeaderData(lines[0])
data = lines[cc+1:cc+1+h]

# Build the color map

colorMap = {}

for l in lines[1:1+cc] :
    idx = l.rfind("c #")

    if idx == -1 :
        continue

    key = l[0]
    value = l[idx+3:]

    colorMap[key] = Color(value)

print len(data)
for d in data :
    c = d[0]
    print colorMap[c].toColor()
