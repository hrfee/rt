import csv, sys

pairs = {}
header = []

with open(sys.argv[1], "r") as f:
    csvf = csv.reader(f)
    for l in csvf:
        cc = l[13]
        rt = l[10]
        if l[0] == "Map Name":
            header = l
            continue

        if cc not in pairs:
            pairs[cc] = {"lines": [], "l2": [], "r": 0, "b": 0}

        pairs[cc]["lines"].append(l)
        pairs[cc]["r"] += float(rt)

with open(sys.argv[2], "r") as f:
    csvf = csv.reader(f)
    for l in csvf:
        cc = l[13]
        rt = l[10]
        if l[0] == "Map Name":
            header = l
            continue

        if cc not in pairs:
            pairs[cc] = {"lines": [], "l2": [], "r": 0, "b": 0}

        pairs[cc]["l2"].append(l)
        pairs[cc]["b"] += float(rt)

print ("CC to render time")

for cc in pairs:
    l = pairs[cc]["lines"][0]
    avg = pairs[cc]["r"] / len(pairs[cc]["lines"])
    avgb = pairs[cc]["b"] / len(pairs[cc]["l2"])
    print(f"{l[13]} & {512 % int(l[13])} & {avg} & {avgb}\\\\")
    print("\\midrule")
