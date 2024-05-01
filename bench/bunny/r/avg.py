import csv, sys

pairs = {}
header = []

with open(sys.argv[1], "r") as f:
    csvf = csv.reader(f)
    for l in csvf:
        mapopt = l[0] + l[14]
        if l[0] == "Map Name":
            header = l
            continue

        if mapopt not in pairs:
            pairs[mapopt] = {"lines": [], "r": 0, "b": 0}

        pairs[mapopt]["lines"].append(l)
        pairs[mapopt]["r"] += float(l[10])
        if l[16] != "":
            pairs[mapopt]["b"] += float(l[16])

avgs = {}

header[10] = "Average render time (ms)"
header[16] = "Average structure build time (ms)"

print(",".join(header)[:-1])

groups = []

group = []
for mapopt in pairs:
    line = pairs[mapopt]["lines"][0]
    line[10] = str(pairs[mapopt]["r"] / len(pairs[mapopt]["lines"]))
    if pairs[mapopt]["b"] > 0:
        line[16] = str(pairs[mapopt]["b"] / len(pairs[mapopt]["lines"]))
    else:
        line[16] = "N/A"
    
    if len(group) < 2:
        group.append(line)

    if len(group) >= 2:
        groups.append(group)
        group = []

for g in groups:
    fname = g[0][0].replace("bench/bunny/m/", "")
    print(f"{fname} & {g[0][1]} & {g[0][10]} & {g[1][10]} & {g[1][16]}\\\\")
    print("\\midrule")

print("Triangle Count, Render Time (ms, No BVH)")
for g in groups:
    print(f"{g[0][1]},{g[0][10]}")

print("Triangle Count, Render Time (ms, BVH)")
for g in groups:
    print(f"{g[1][1]},{g[1][10]}")

print("Triangle Count, Build Time (ms)")
for g in groups:
    print(f"{g[1][1]},{g[1][16]}")
