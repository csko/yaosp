import sys

class Color :
    def __init__(self, c) :
        self.r = int("0x%s" % c[0:2], 16)
        self.g = int("0x%s" % c[2:4], 16)
        self.b = int("0x%s" % c[4:6], 16)

    def toColor(self) :
        return "yguipp::Color(%d, %d, %d)" % (self.r, self.g, self.b)

    def toArray(self, addAlpha = True) :
        a = [self.r, self.g, self.b]
        if addAlpha :
            a += [255]
        return a

def extractHeaderData(header) :
    s = header.split(" ")
    return ( int(s[0]), int(s[1]),
             int(s[2]), int(s[3]) )

if len(sys.argv) < 3 :
    sys.exit(1)

# Read the whole XPM data from the file

f = open(sys.argv[2], "r")
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

mode = sys.argv[1]

if mode == "column" :
    print len(data)
    for d in data :
        c = d[0]
        print colorMap[c].toColor()
elif mode == "image" :
    a = []
    for d in data :
        for c in d :
            a += colorMap[c].toArray()

    i = 0
    s = "    "
    for c in a :
        s += "%3d, " % c
        i += 1
        if i == 25 :
            s += "\n    "
            i = 0
    while s[-1] in ["\n", ",", " "] :
        s = s[:-1]

    print s
